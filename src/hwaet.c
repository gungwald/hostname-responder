/* hwæt */
#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset */
#include <unistd.h>     /* close */
#include <libgen.h>	/* basename */
#include <errno.h>	/* errno */
#include <limits.h>	/* POSIX HOST_NAME_MAX */

#include "cross-platform-sockets.h"
#include "hwaet-common.h"


bool initSubnetBroadcastAddress(struct sockaddr_in *address);
void initLocalReceiptPortAddress(struct sockaddr_in *address);
bool sendHostnameBrodcastRequest(int socket);
void readResponses(int socket);
bool findBroadcastAddr(struct sockaddr_in *addr);
bool isPrimaryInterface(struct ifaddrs *i);
bool isLoopback(struct sockaddr *address);
bool findPrimaryInterface(struct ifaddrs *i);


int main(int argc, char *argv[])
{
    int bcastSock, responseSock;

    programName = basename(argv[0]);
    noErrors = true;

#ifdef DEBUG    
    printInterfaces();
#endif
    
    if ((bcastSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        if (sendHostnameBrodcastRequest(bcastSock)) {
            if ((responseSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
                readResponses(responseSock);
                close(responseSock);
            } else {
                handleError("Failed to create socket for receiving hostname responses", NULL, errno);
            }
        }
        close(bcastSock);
    } else {
        handleError("Failed to create socket for broadcasting hostname request", NULL, errno);
    }
    return noErrors ? EXIT_SUCCESS : EXIT_FAILURE;
}

void readResponses(int sock)
{
    char msgRecvd[HOST_NAME_MAX+1];
    size_t msgSz;
    struct sockaddr_in remoteAddr;
    socklen_t addrSz;
    int cnt;
    struct sockaddr_in myPort;
    struct timeval timeout = {.tv_sec=20, .tv_usec=0};

    msgSz = sizeof(msgRecvd);
    addrSz = sizeof(remoteAddr);
    initLocalReceiptPortAddress(&myPort);
    
    if (bind(sock, (sa*) &myPort, addrSz) != SOCK_ERR) {
        /* Set the read timeout on the socket so it doesn't hang waiting forever. */
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != SOCK_ERR) {
            while ((cnt = recvfrom(sock, msgRecvd, msgSz, 0, (sa*) &remoteAddr, &addrSz)) != SOCK_ERR && cnt > 0) {
	        msgRecvd[cnt] = '\0';
                puts(msgRecvd);
            }
	    if (cnt == SOCK_ERR) {
	        if (errno == EAGAIN) {
	    	    printf("Timed out waiting for responses\n");
	        } else {
	            handleError("Failed to read responses", NULL, errno);
		}
	    }
        } else {
	    handleError("Failed to set timeout for reading responses from subnet machines", NULL, errno);
	}
    } else {
        handleError("Failed to bind to local port", NULL, errno);
    }
}

bool sendHostnameBrodcastRequest(int sock)
{
    const int bcastOn = 1; /* The value that the socket option will be set to. A value of one turns it on. */
    struct sockaddr_in bcastAddr;
    char *msg = "REPLY WITH HOSTNAME";

    /* Configure it to broadcast to multiple receivers. */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcastOn, sizeof(bcastOn)) != SOCK_ERR) {
        if (initSubnetBroadcastAddress(&bcastAddr)) {
            /* Broadcast the data to the whole subnet.*/
	    if (sendto(sock, msg, strlen(msg), 0, (sa*) &bcastAddr, sizeof(bcastAddr)) != SOCK_ERR) {
                ;
            } else {
                handleError("Failed to broadcast host name request", NULL, errno);
	    }
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
bool initSubnetBroadcastAddress(struct sockaddr_in *bcAddr)
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
	for (iface = ifaceList; iface != NULL; iface = iface->ifa_next) {
            if (isPrimaryInterface(iface)) {
	    	*result = *iface; /* Copy whole struct from system to result. */
		found = true;
        	break;		  /* Exit the for loop because it was found.  */
            }
	}
	freeifaddrs(ifaceList);
	if (! found) {
            handleError("Failed to find primary interface", NULL, 0);
	}
    } else {
        handleError("Failed to get network interfaces", NULL, errno);
    }
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
