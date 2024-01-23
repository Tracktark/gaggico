#include "discovery.hpp"
#include <string.h>
#include <pico/cyw43_arch.h>
#include <lwip/udp.h>
#include "config.hpp"

using namespace discovery;

static void incoming_callback(void* arg, struct udp_pcb* pcb, struct pbuf* inp, const ip_addr_t* addr, u16_t port) {
    pbuf_free(inp);

    static struct pbuf* outp = nullptr;
    if (outp == nullptr) {
        outp = pbuf_alloc(PBUF_TRANSPORT, strlen(RESPONSE), PBUF_ROM);
        outp->payload = (char*) RESPONSE;
    }
    udp_sendto(pcb, outp, addr, PORT);
}

void discovery::init() {
    cyw43_arch_lwip_begin();
    struct udp_pcb* pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, PORT);
    udp_recv(pcb, incoming_callback, nullptr);
    cyw43_arch_lwip_end();
}

