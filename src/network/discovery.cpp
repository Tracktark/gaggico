#include "discovery.hpp"
#include <string.h>
#include <pico/cyw43_arch.h>
#include <lwip/udp.h>
#include "config.hpp"

using namespace discovery;

const char* RESPONSE = "pico here";
static struct pbuf* resp_pbuf = nullptr;

static void incoming_callback(void* arg, struct udp_pcb* pcb, struct pbuf* inp, const ip_addr_t* addr, u16_t port) {
    pbuf_free(inp);
    udp_sendto(pcb, resp_pbuf, addr, PORT);
}

void discovery::init() {
    cyw43_arch_lwip_begin();

    resp_pbuf = pbuf_alloc(PBUF_TRANSPORT, strlen(RESPONSE), PBUF_ROM);
    resp_pbuf->payload = (char*) RESPONSE;

    struct udp_pcb* pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, PORT);
    udp_recv(pcb, incoming_callback, nullptr);

    cyw43_arch_lwip_end();
}
