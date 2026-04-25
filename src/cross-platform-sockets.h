#ifndef CROSS_PLATFORM_SOCKETS_H
#define CROSS_PLATFORM_SOCKETS_H



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
    #include <sys/socket.h> /* socket, bind, getifaddrs, freeifaddrs */
    #include <sys/types.h>  /* getifaddrs, freeifaddrs */
    #include <netinet/in.h> /* sockaddr_in, in_port_t, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
    #include <arpa/inet.h>  /* inet_ntop */
    #include <net/if.h>	    /* IFF_BROADCAST */
    #include <ifaddrs.h>    /* getifaddrs, freeifaddrs, ifaddrs */
    #include <limits.h>     /* POSIX HOST_NAME_MAX */
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
#endif


int sockInit(void);
int sockQuit(void);
int sockClose(SOCKET sock);


#endif
