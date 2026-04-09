/* hwæt */
#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset */
#include <unistd.h>     /* close */
#include <libgen.h>	/* basename */
#include <errno.h>	/* errno */

#include <sys/types.h>	/* getifaddrs, freeifaddrs */
#include <sys/socket.h> /* socket, bind, getifaddrs, freeifaddrs, sockaddr, sa_family_t */
#include <ifaddrs.h>	/* getifaddrs, freeifaddrs, ifaddrs */
#include <net/if.h>	/* IFF_BROADCAST */

/* Figure out if compiling under BSD and include the header for sockaddr_in. */
#if defined(__unix__)
  #include <sys/param.h>
  #if defined(BSD)
    #include <netinet/in.h> /* sockaddr_in */
  #endif
#endif

#include "hwaet-common.h"

bool isLoopback(struct sockaddr *address);
bool isPrimaryInterface(struct ifaddrs *i);
struct sockaddr_in *findIPv4Broadcast();
void initSubnetBroadcastAddress(struct sockaddr_in *address);
void initLocalReceiptPortAddress(struct sockaddr_in *address);
void sendHostnameBrodcastRequest(int socket);
void readResponses(int socket);



int main(int argc, char *argv[])
{
    int bcastSock, responseSock;

    programName = basename(argv[0]);
    noErrors = true;

#ifdef DEBUG    
    dumpInterfaces();
#endif
    
    if ((bcastSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        sendHostnameBrodcastRequest(bcastSock);
        if ((responseSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
            readResponses(responseSock);
            close(responseSock);
        } else {
            handleError("Failed to create socket for receiving hostname responses", NULL, errno);
        }
        close(bcastSock);
    } else {
        handleError("Failed to create socket for broadcasting hostname request", NULL, errno);
    }
    return noErrors ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool isLoopback(struct sockaddr *address)
{
    bool isLoopback = false;
    struct sockaddr_in *inetAddress;

    if (address->sa_family == AF_INET) {
        inetAddress = (struct sockaddr_in *) address;
        if (inetAddress->sin_addr.s_addr == INADDR_LOOPBACK) {
            isLoopback = true;
        }
    }
    return isLoopback;
}

bool isPrimaryInterface(struct ifaddrs *iface)
{
    return iface->ifa_addr != NULL
           && iface->ifa_addr->sa_family == AF_INET
           && (iface->ifa_flags & IFF_BROADCAST)
           && !isLoopback(iface->ifa_addr);
}

struct sockaddr_in *findIPv4Broadcast()
{
    static struct sockaddr_in bcastAddr;
    struct ifaddrs *ifaceList; /* Required for freeifaddrs */
    struct ifaddrs *iface;
    struct sockaddr_in *bcastAddrSysPtr;

    if (getifaddrs(&ifaceList) != SOCK_ERR) {
	for (iface = ifaceList; iface != NULL; iface = iface->ifa_next) {
            if (isPrimaryInterface(iface)) {
        	bcastAddrSysPtr = (struct sockaddr_in *) iface->ifa_broadaddr;
        	/* Copy the whole broadcast address structure into static variable. */
        	bcastAddr = *bcastAddrSysPtr;
	        printf("Broadcasting to %s on interface %s...\n", addr2Str((sa*)&bcastAddr), iface->ifa_name);
		/* The broadcast address was found, so exit the loop. */
        	break;
            }
	}
	freeifaddrs(ifaceList);
    } else {
        handleError("Failed to get network interfaces", NULL, errno);
    }
    return &bcastAddr;
}

void readResponses(int sock)
{
    char msgRecvd[MAX_HOSTNAME_LEN+1];
    struct sockaddr_in remoteAddr;
    socklen_t addrSz;
    int cnt;
    struct sockaddr_in myPort;
    struct timeval timeout = {.tv_sec=20, .tv_usec=0};

    initLocalReceiptPortAddress(&myPort);
    addrSz = sizeof(remoteAddr);
    
    if (bind(sock, (sa*) &myPort, addrSz) != SOCK_ERR) {
        /* Set the read timeout on the socket so it doesn't hang waiting forever. */
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != SOCK_ERR) {
            while ((cnt = recvfrom(sock, msgRecvd, MAX_HOSTNAME_LEN, 0, (sa*) &remoteAddr, &addrSz)) != SOCK_ERR && cnt > 0) {
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

void sendHostnameBrodcastRequest(int sock)
{
    int bcastOn = 1; /* The value that the socket option will be set to. A value of one turns it on. */
    struct sockaddr_in bcastAddr;
    char *msg = "REPLY WITH HOSTNAME";

    /* Configure it to broadcast to multiple receivers. */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcastOn, sizeof(bcastOn)) != SOCK_ERR) {
        initSubnetBroadcastAddress(&bcastAddr);
        /* Broadcast the data to the whole subnet.*/
	if (sendto(sock, msg, strlen(msg), 0, (sa*) &bcastAddr, sizeof(bcastAddr)) != SOCK_ERR) {
            printf("Send broadcast message to subnet\n");
        } else {
            handleError("Failed to broadcast host name request", NULL, errno);
	}
    } else {
        handleError("Failed to configure socket to broadcast", NULL, errno);
    }
}

/**
 * Build the address for the intended receivers, which are all hosts on this
 * subnet.
 */
void initSubnetBroadcastAddress(struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(findIPv4Broadcast()->sin_addr.s_addr);
    address->sin_port = htons(SERVER_PORT);
}

void initLocalReceiptPortAddress(struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(INADDR_ANY);
    address->sin_port = htons(CLIENT_PORT);
}
