/* hw�t */
#include <stdio.h>   /* printf, fprintf */
#include <stdbool.h> /* bool, true, false */
#include <stdlib.h>  /* atoi, exit, EXIT_FAILURE */
#include <string.h>  /* memset */
#include <unistd.h>  /* close */
#include <libgen.h>  /* basename */
#include <errno.h>   /* errno */
#include <limits.h>  /* POSIX HOST_NAME_MAX */

#include "cross-platform-sockets.h"
#include "hwaet-common.h"


bool initBroadcastAddr(struct sockaddr_in *address);
void initLocalReceiptPortAddress(struct sockaddr_in *address);
bool broadcastMessage(int socket, const char *message);
void readResponses(int socket);
bool findBroadcastAddr(struct sockaddr_in *addr);
bool isPrimaryInterface(struct ifaddrs *i);
bool isLoopback(struct sockaddr *address);
bool findPrimaryInterface(struct ifaddrs *i);

char *programName = NULL;
bool noErrors = true;
const char *message = "Hw\xE6t"; /* Old English for what, who, why or possibly other things. */

int main(int argc, char *argv[])
{
  int bcSock, respSock;

  programName = basename(argv[0]);
  noErrors = true;

#ifdef DEBUG
  printInterfaces();
#endif

  if ((bcSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
    if (broadcastMessage(bcSock, message)) {
      if ((respSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        readResponses(respSock);
        close(respSock);
      } else
        handleError("Failed to create socket for receiving responses", NULL, errno);
    }
    close(bcSock);
  } else {
    handleError("Failed to create socket for broadcasting requests", NULL, errno);
  }
  return noErrors ? EXIT_SUCCESS : EXIT_FAILURE;
}

void readResponses(int sock)
{
  size_t msgSz;
  struct sockaddr_in remoteAddr;
  struct sockaddr_in localAddr;
  socklen_t addrSz;
  socklen_t timeoutSz;
  int cnt;
  struct timeval timeout = {.tv_sec=20, .tv_usec=0};

  msgSz = sizeof(msgRecvd);
  addrSz = (socklen_t) sizeof(struct sockaddr_in);
  timeoutSz = (socklen_t) sizeof(timeout);
  initLocalReceiptPortAddress(&localAddr);

  if (bind(sock, (sa*) &localAddr, addrSz) != SOCK_ERR) {
    /* Set the read timeout on the socket so it doesn't hang waiting forever. */
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, timeoutSz) != SOCK_ERR) {
      while ((cnt = recvfrom(sock, msgRecvd, msgSz, 0, (sa*) &remoteAddr, &addrSz)) != SOCK_ERR && cnt > 0) {
        msgRecvd[cnt] = '\0';
        puts(msgRecvd);
      }
      if (cnt == SOCK_ERR) {
        if (errno == EAGAIN)
          printf("Timed out waiting for responses\n");
        else
          handleError("Failed to read responses", NULL, errno);
      }
    } else
      handleError("Failed to set timeout for reading responses", NULL, errno);
  } else
    handleError("Failed to bind to local port", NULL, errno);
}

bool broadcastMessage(int sock, const char *msg)
{
  const int bcOn = 1; /* The socket option value that will turn broadcast on. */
  const int NO_FLAGS = 0;
  struct sockaddr_in bcAddr; /* Broadcast address */
  size_t msgSz; /* Size of message in bytes */
  socklen_t bcAddrSz; /* Size of bcAddr */
  socklen_t bcOnSz; /* Size of bcOn */

  msgSz = strlen(msg); /* Only works because a char is 1 byte. */
  bcOnSz = (socklen_t) sizeof(bcOn);
  bcAddrSz = (socklen_t) sizeof(bcAddr);

  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcOn, bcOnSz) != SOCK_ERR) {
    if (initBroadcastAddr(&bcAddr)) {
      if (sendto(sock, msg, msgSz, NO_FLAGS, (sa*) &bcAddr, bcAddrSz) != SOCK_ERR)
        printf("Broadcast '%s' to local subnet.\n", msg);
      else
        handleError("Failed to broadcast request", NULL, errno);
    }
  } else {
    handleError("Failed to configure socket to broadcast", NULL, errno);
  }
  return noErrors;
}

/**
 * Build the address for the intended receivers, which are all hosts on this
 * subnet.
 */
bool initBroadcastAddr(struct sockaddr_in *bcAddr)
{
  struct sockaddr_in addr;

  if (findBroadcastAddr(&addr)) {
    memset(bcAddr, 0, sizeof(struct sockaddr_in));
    bcAddr->sin_family = AF_INET;
    bcAddr->sin_addr.s_addr = addr.sin_addr.s_addr;
    bcAddr->sin_port = htons(SERVER_PORT);
  }
  return noErrors;
}

void initLocalReceiptPortAddress(struct sockaddr_in *address)
{
  memset(address, 0, sizeof(*address));
  address->sin_family = AF_INET;
  address->sin_addr.s_addr = htonl(INADDR_ANY);
  address->sin_port = htons(CLIENT_PORT);
}

bool isPrimaryInterface(struct ifaddrs *iface)
{
  return iface->ifa_addr != NULL
         && iface->ifa_addr->sa_family == AF_INET
         && (iface->ifa_flags & IFF_BROADCAST)
         && !isLoopback(iface->ifa_addr);
}

bool isLoopback(struct sockaddr *addr)
{
  return addr->sa_family == AF_INET
         && ((struct sockaddr_in *) addr)->sin_addr.s_addr == INADDR_LOOPBACK;
}

bool findPrimaryInterface(struct ifaddrs *result)
{
  struct ifaddrs *ifaceList; /* Required for freeifaddrs */
  struct ifaddrs *iface;
  bool found = false;

  if (getifaddrs(&ifaceList) != SOCK_ERR) {
    for (iface = ifaceList; found!=true && iface!=NULL; iface = iface->ifa_next) {
      if (isPrimaryInterface(iface)) {
        *result = *iface; /* Copy whole struct from system to result. */
        found = true;
      }
    }
    freeifaddrs(ifaceList);
    if (! found)
      handleError("Failed to find primary interface", NULL, 0);
  } else
    handleError("Failed to get network interfaces", NULL, errno);
  return noErrors;
}

bool findBroadcastAddr(struct sockaddr_in *bcastAddr /* out */)
{
  struct ifaddrs primeIface;

  if (findPrimaryInterface(&primeIface)) {
    *bcastAddr = *((struct sockaddr_in *)primeIface.ifa_broadaddr); /* Copy whole struct */
  }
  return noErrors;
}
