#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#define NO_SYS                          1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        8000

#define LWIP_TCP                        1
#define TCP_MSS                         1460
#define TCP_WND                         (4 * TCP_MSS)
#define TCP_SND_BUF                     (4 * TCP_MSS)

#define LWIP_DHCP                       1
#define LWIP_DNS                        1

#define LWIP_MQTT                       1

#endif