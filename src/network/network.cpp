#include "network.hpp"
#include <pico/cyw43_arch.h>
#include <pico/util/queue.h>
#include <lwip/tcp.h>
#include <lwip/pbuf.h>
#include <variant>
#include "config.hpp"
#include "impl/message.hpp"
#include "impl/queue.hpp"
#include "lwip/err.h"
#include "messages.hpp"
#include "panic.hpp"

// #define LOG(...) printf(__VA_ARGS__)
#define LOG(...)

#ifndef COUNTRY_CODE
#define COUNTRY_CODE PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE
#endif

using namespace network;

constexpr auto IN_MESSAGE_BUFFER_CAP = 127;
constexpr auto OUT_MESSAGE_BUFFER_CAP = 127;
struct Client {
    tcp_pcb* pcb;
    isize current_msg_len;
    usize recved_len;
    usize written_len;
    usize acked_len;
    u8 message_buffer[IN_MESSAGE_BUFFER_CAP];
};

static Client clients[CLIENT_CAPACITY] = {0};

static u8 out_message_buffer[OUT_MESSAGE_BUFFER_CAP];
static isize out_message_len = -1;
static Queue<OutMessages, 10> out_message_queue;

static err_t close_connection(tcp_pcb* pcb) {
    err_t err = tcp_close(pcb);
    if (err != ERR_OK) {
        printf("Connection close failed, aborting: %d\n", err);
        tcp_abort(pcb);
        return ERR_ABRT;
    }
    printf("Connection closed\n");
    return ERR_OK;
}

static bool try_receive(Client& client, pbuf* p, usize size, u32& offset) {
    u32 copied = pbuf_copy_partial(p,
                                   client.message_buffer + client.recved_len,
                                   size - client.recved_len,
                                   offset);
    offset += copied;
    client.recved_len += copied;
    tcp_recved(client.pcb, copied);
    return client.recved_len == size;
}

static err_t recv_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    Client& client = *static_cast<Client*>(arg);

    if (!p) {
        client.pcb = nullptr;
        return close_connection(tpcb);
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        printf("Error when receiving data: %d\n", err);
        return err;
    }
    u32 offset = 0;

    while (p->tot_len - offset > 0) {
        if (client.current_msg_len == -1) {
            if (try_receive(client, p, sizeof(u32), offset)) {
                client.recved_len = 0;
                client.current_msg_len = ntohl(reinterpret_cast<u32*>(client.message_buffer)[0]);
            }
        } else {
            if (try_receive(client, p, client.current_msg_len, offset)) {
                u32 msg_len = client.current_msg_len - 4;
                client.recved_len = 0;
                client.current_msg_len = -1;

                u32 msg_id = ntohl(reinterpret_cast<u32*>(client.message_buffer)[0]);
                u8* msg_data = client.message_buffer + sizeof(u32);
                LOG("Received msg %u from %u\n", msg_id, (&client - clients));
                for (int i = 0; i < msg_len; i++) {
                    LOG("%02X ", msg_data[i]);
                }
                LOG("\n");
                handle_incoming_msg<InMessages>(msg_id, msg_data);
            }
        }
    }

    pbuf_free(p);
    return ERR_OK;
}

static void err_callback(void* arg, err_t err) {
    Client& client = *static_cast<Client*>(arg);
    client.pcb = nullptr;
    printf("Error on connection %u: %d\n", (&client - clients), err);
}

static err_t sent_callback(void* arg, tcp_pcb* tpcb, u16 len) {
    Client& client = *static_cast<Client*>(arg);
    u32 id = (&client - clients);
    client.acked_len += len;
    LOG("Client %u received %u out of %u bytes\n", id, client.acked_len, out_message_len);
    return ERR_OK;
}

static err_t new_connection_callback(void* arg, struct tcp_pcb* client_pcb, err_t err) {
    if (err != ERR_OK || client_pcb == NULL) {
        printf("Error on new connection: %d\n", err);
        return ERR_VAL;
    }

    Client* client = nullptr;
    for (std::size_t i = 0; i < CLIENT_CAPACITY; ++i) {
        if (clients[i].pcb == nullptr) {
            client = &clients[i];
            break;
        }
    }
    if (!client) {
        printf("No capacity for new client\n");
        return close_connection(client_pcb);
    }
    client->pcb = client_pcb;
    client->current_msg_len = -1;
    client->recved_len = 0;
    client->acked_len = 0;
    client->written_len = 0;

    printf("New connection\n");
    tcp_arg(client_pcb, client);
    tcp_sent(client_pcb, sent_callback);
    tcp_recv(client_pcb, recv_callback);
    tcp_err(client_pcb, err_callback);
    return ERR_OK;
}

static void try_send() {
    cyw43_arch_lwip_begin();
    bool done_sending = true;
    for (usize i = 0; i < CLIENT_CAPACITY; i++) {
        Client& client = clients[i];
        if (client.pcb == nullptr) continue;

        if (client.acked_len < out_message_len)
            done_sending = false;

        u32 send_buffer_size = tcp_sndbuf(client.pcb);
        if (client.written_len < out_message_len && send_buffer_size > 0) {
            u32 write_size = out_message_len - client.written_len;
            if (write_size > send_buffer_size) {
                write_size = send_buffer_size;
            }

            err_t err = tcp_write(client.pcb, out_message_buffer + client.written_len, write_size, 0);
            if (err == ERR_CONN) {
                printf("Client not connected, closing connection\n");
                tcp_pcb* pcb = client.pcb;
                client.pcb = nullptr;
                close_connection(pcb);
                continue;
            } else if (err != ERR_OK) {
                printf("tcp_write failed: %d\n", err);
                continue;
            }
            client.written_len += write_size;

            err = tcp_output(client.pcb);
            if (err != ERR_OK) {
                printf("Couldn't output tcp: %d\n", err);
                continue;
            }
        }
    }

    if (done_sending) {
        LOG("End message\n");
        out_message_queue.pop_blocking();
        out_message_len = -1;
    }

    cyw43_arch_lwip_end();
}

static void serialize_message() {
    const OutMessages& msg = out_message_queue.peek();

    u8* start = out_message_buffer + sizeof(u32);
    u8* end = start;

    std::visit([&end](auto&& msg) {
        using Msg = std::decay_t<decltype(msg)>;
        write_val(end, Msg::OUTGOING_ID);
        write_struct(msg, end);
    }, msg);

    u32 msglen_n = htonl(end-start);
    memcpy(out_message_buffer, &msglen_n, sizeof(u32));
    out_message_len = end-out_message_buffer;
}

void network::process_outgoing_messages() {
    if (out_message_len > 0) { // Currently sending a message
        try_send();
        return;
    }
    if (out_message_queue.size() > 0) { // Sending a new message
        LOG("Begin message\n");
        for (usize i = 0; i < CLIENT_CAPACITY; i++) {
            if (clients[i].pcb == nullptr) continue;
            clients[i].acked_len = 0;
            clients[i].written_len = 0;
        }
        serialize_message();
    }
}

void network::enqueue_message(const OutMessages& msg) {
    if (!out_message_queue.try_push(msg)) {
        panic(Error::MESSAGE_QUEUE_FULL);
    }

    LOG("Enqueuing message, queue size: %u\n", out_message_queue.size());
}

void network::wifi_init() {
    if (cyw43_arch_init_with_country(COUNTRY_CODE)) {
        panic(Error::WIFI_INIT_FAILED);
    };
    cyw43_arch_enable_sta_mode();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30 * 1000)) {
        panic(Error::WIFI_COULDNT_CONNECT);
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
}

void network::server_init() {
    cyw43_arch_lwip_begin();

    out_message_queue.init();

    struct tcp_pcb* pcb = tcp_new();
    tcp_bind(pcb, IP4_ADDR_ANY, PORT);
    pcb = tcp_listen_with_backlog(pcb, CLIENT_CAPACITY);

    tcp_accept(pcb, new_connection_callback);

    cyw43_arch_lwip_end();
}
