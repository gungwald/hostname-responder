#ifndef CROSS_PLATFORM_SOCKETS_H
#define CROSS_PLATFORM_SOCKETS_H

/* Bind, the standard internet name server software, claims that RFC883 claims
   that a Fully Qualified Domain Name (FQDN) can be 1025 bytes. This doesn't
   seem correct, as every other source says it's 255 bytes. But, it is a safe
   number because it's the biggest option. See /usr/include/arpa/nameser.h. */
#define FQDN_MAX_LEN 1025

/* Just the string with no terminator, useful in for stmt comparisons */
#define MAC_ADDR_STR_LEN 17
/* ff:FF:FF:FF:FF:FF plus string terminator char */
#define MAC_ADDR_STR_SIZ (MAC_ADDR_STR_LEN+1)
/* The string, plus line terminators for reading with fgets, and string terminator */
#define MAC_ADDR_LINE_SIZ (MAC_ADDR_STR_SIZ+2)

/**
 * The sentinel value all socket functions return this when they fail.
 */
#define SOCK_ERR -1

/**
 * The "struct sockaddr" type has to be used in many casts. Defining it to
 * something shorter is very helpful.
 */
typedef struct sockaddr sa;         /* type sa    = struct sockaddr     */
typedef struct sockaddr_in sain;    /* type sain  = struct sockaddr_in  */
typedef struct sockaddr_in6 sain6;  /* type sain6 = struct sockaddr_in6 */

/* Source - https://stackoverflow.com/a/28031039
   Posted by user4200092, modified by community. See post 'Timeline' for change history
   Retrieved 2026-04-23, License - CC BY-SA 3.0 */
#ifdef _WIN32
  /* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
  #include <winsock2.h>
  #include <Ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
#else
  #include <sys/socket.h>       /* socket, bind, getifaddrs, freeifaddrs */
  #include <sys/types.h>        /* getifaddrs, freeifaddrs */
  #include <net/if_arp.h>       /* Required for types in netinet/if_ether.h */
  #include <netinet/in.h>       /* sockaddr_in, in_port_t, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
  #include <netinet/if_ether.h> /* ether_aton on BSD */
#ifdef __linux__
  #include <netinet/ether.h>    /* Linux: defines ether_aton, BSD: file nonexistant */
#endif
  #include <arpa/inet.h>        /* inet_ntop */
  #include <net/if.h>	        /* IFF_BROADCAST */
  #include <ifaddrs.h>          /* getifaddrs, freeifaddrs, ifaddrs */
  #include <limits.h>           /* POSIX HOST_NAME_MAX */
  #include <unistd.h>
  #define SOCKET int
  #define INVALID_SOCKET -1
#endif


int sockInit(void);
int sockQuit(void);
int sockClose(SOCKET sock);


#endif
