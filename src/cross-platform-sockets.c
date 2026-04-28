#include "cross-platform-sockets.h"

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

