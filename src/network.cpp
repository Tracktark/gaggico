#include "network.hpp"
#include <array>
#include <pico/cyw43_arch.h>
#include <lwip/tcp.h>
#include <lwip/pbuf.h>
#include "config.hpp"
#include "messages.hpp"
#include "panic.hpp"

#ifndef COUNTRY_CODE
#define COUNTRY_CODE PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE
#endif

using namespace network;

tcp_pcb* clients[CLIENT_CAPACITY] = {0};
std::array<u8, 127> out_message_buffer;
std::array<u8, 127> in_message_buffer;

static err_t close_connection(struct tcp_pcb* tpcb) {
    for (std::size_t i = 0; i < CLIENT_CAPACITY; ++i) {
        if (clients[i] == tpcb) {
            clients[i] = nullptr;
            break;
        }
    }

    err_t err = tcp_close(tpcb);
    if (err != ERR_OK) {
        printf("Connection close failed, aborting: %d\n", err);
        tcp_abort(tpcb);
        return ERR_ABRT;
    }
    printf("Connection closed\n");
    return ERR_OK;
}

static err_t incoming_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (!p) {
        return close_connection(tpcb);
    }

    pbuf_copy_partial(p, in_message_buffer.data(), 2*sizeof(u32), 0);
    u32* data = (u32*)in_message_buffer.data();

    u32 len = ntohl(data[0]) - sizeof(u32);
    u32 msg_id = ntohl(data[1]);

    pbuf_copy_partial(p, in_message_buffer.data(), len, 8);
    u8* msg_data = in_message_buffer.data();

    InMessage* msg;
    switch (msg_id) {
    case PowerMessage::ID:
        msg = new PowerMessage;
        break;
    }

    msg->read(msg_data);
    msg->handle();

    delete msg;

    pbuf_free(p);
    return ERR_OK;
}

static err_t new_connection_callback(void* arg, struct tcp_pcb* client_pcb, err_t err) {
    if (err != ERR_OK || client_pcb == NULL) {
        printf("Error on new connection: %d\n", err);
        return ERR_VAL;
    }

    bool no_capacity = true;
    for (std::size_t i = 0; i < CLIENT_CAPACITY; ++i) {
        if (clients[i] == nullptr) {
            clients[i] = client_pcb;
            no_capacity = false;
            break;
        }
    }
    if (no_capacity) {
        printf("No capacity for new client\n");
        return close_connection(client_pcb);
    }

    printf("New connection\n");
    tcp_recv(client_pcb, incoming_callback);
    return ERR_OK;
}

void network::send(const OutMessage& msg) {
    cyw43_arch_lwip_begin();
    u8* start = out_message_buffer.data() + sizeof(u32);
    u8* end = start;
    msg.write(end);
    u32 msglen = end - start;
    u32 msglen_n = htonl(msglen);
    memcpy(out_message_buffer.data(), &msglen_n, sizeof(u32));

    u32 len = msglen + sizeof(u32);

    for (std::size_t i = 0; i < CLIENT_CAPACITY; ++i) {
        if (clients[i] == nullptr) continue;

        err_t err = tcp_write(clients[i], out_message_buffer.data(), len, 0);
        if (err != ERR_OK) {
            if (err == ERR_CONN) {
                printf("Client not connected, closing connection\n");
                close_connection(clients[i]);
                continue;
            }
            printf("Couldn't write tcp: %d\n", err);
            continue;
        }
        err = tcp_output(clients[i]);
        if (err != ERR_OK) {
            printf("Couldn't output tcp: %d\n", err);
            continue;
        }
    }
    cyw43_arch_lwip_end();
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

    struct tcp_pcb* pcb = tcp_new();
    tcp_bind(pcb, IP4_ADDR_ANY, PORT);
    pcb = tcp_listen_with_backlog(pcb, BACKLOG);

    tcp_accept(pcb, new_connection_callback);

    cyw43_arch_lwip_end();
}
