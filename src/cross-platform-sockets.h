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

/* Source - https://stackoverflow.com/a/28031039
   Posted by user4200092, modified by community. See post 'Timeline' for change history
   Retrieved 2026-04-23, License - CC BY-SA 3.0 */

int sockInit(void)
{
  #ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(1,1), &wsa_data);
  #else
    return 0;
  #endif
}

int sockQuit(void)
{
  #ifdef _WIN32
    return WSACleanup();
  #else
    return 0;
  #endif
}


// Source - https://stackoverflow.com/a/28031039
// Posted by user4200092, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-23, License - CC BY-SA 3.0

/* Note: For POSIX, typedef SOCKET as an int. */

int sockClose(SOCKET sock)
{

  int status = 0;

  #ifdef _WIN32
    status = shutdown(sock, SD_BOTH);
    if (status == 0) { status = closesocket(sock); }
  #else
    status = shutdown(sock, SHUT_RDWR);
    if (status == 0) { status = close(sock); }
  #endif

  return status;

}


#endif
