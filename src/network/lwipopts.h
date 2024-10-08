#ifndef _LWIPOPTS_EXAMPLE_COMMONH_H
#define _LWIPOPTS_EXAMPLE_COMMONH_H


// Settings taken from pico w examples
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html for details)

#define NO_SYS                      1
#define LWIP_SOCKET                 0
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1
#else
#define MEM_LIBC_MALLOC             0
#endif
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define TCP_WND                     (2 * TCP_MSS)
#define TCP_MSS                     256
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN                0
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0
// #define ETH_PAD_SIZE                2
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0


#include <inttypes.h>
#ifdef __cplusplus
extern "C" {
#endif

void ntp_set_system_time(uint32_t sec, uint32_t us);
uint64_t ntp_get_system_time();

#ifdef __cplusplus
}
#endif

#define SNTP_SET_SYSTEM_TIME_US(sec, us) ntp_set_system_time(sec, us)
#define SNTP_GET_SYSTEM_TIME_US(sec, us) do {                           \
        unsigned long long time_us = ntp_get_system_time_us();          \
        (sec) = time_us / 1000000;                                      \
        (us) = time_us % 1000000;                                       \
    } while(0)

#define SNTP_SERVER_DNS 1
#define SNTP_SERVER_ADDRESS "pool.ntp.org"
#define SNTP_COMP_ROUNDTRIP 1
#define SNTP_CHECK_RESPONSE 2
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 1)

#endif /* __LWIPOPTS_H__ */
