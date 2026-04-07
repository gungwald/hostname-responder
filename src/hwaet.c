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
void initReceiptAddress(struct sockaddr_in *address);
enum Outcome sendHostnameBrodcastRequest(int socket);
enum Outcome readResponses(int socket);



char *programName = NULL;


int main(int argc, char *argv[])
{
    int sendSock, recvSock;
    int stat = EXIT_FAILURE;

    programName = basename(argv[0]);

#ifdef DEBUG    
    dumpInterfaces();
#endif
    
    if ((sendSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        if (sendHostnameBrodcastRequest(sendSock)) {
            if ((recvSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
                if (readResponses(recvSock)) {
                    stat = EXIT_SUCCESS;
                }
                close(recvSock);
            } else {
                printError("Failed to create receive socket", errno);
            }
        }
        close(sendSock);
    } else {
        printError("Failed to create send socket", errno);
    }
    return stat;
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
	        printf("Broadcasting to %s on interface %s...\n", sockaddrinToString(&bcastAddr), iface->ifa_name);
		/* The broadcast address was found, so exit the loop. */
        	break;
            }
	}
	freeifaddrs(ifaceList);
    } else {
        printError("Failed to get network interfaces", errno);
    }
    return &bcastAddr;
}

enum Outcome readResponses(int sock)
{
    char resp[MAX_HOSTNAME_LEN+1];
    struct sockaddr_in from;
    socklen_t fromLen;
    int byteCount;
    struct sockaddr_in receiptAddr;
    enum Outcome disposition = FAILURE;
    struct timeval timeout = {.tv_sec=5, .tv_usec=0};

    initReceiptAddress(&receiptAddr);
    fromLen = sizeof(from);
    if (bind(sock, (struct sockaddr *) &receiptAddr, sizeof(receiptAddr)) != SOCK_ERR) {
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != SOCK_ERR) {
            while ((byteCount = recvfrom(sock, resp, MAX_HOSTNAME_LEN, 0, &from, &fromLen)) != SOCK_ERR && byteCount > 0) {
                puts(resp);
            }
	    if (byteCount != SOCK_ERR) {
	        disposition = SUCCESS;
	    } else if (errno == EAGAIN) {
	    	printf("Timed out waiting for responses\n");
	    } else {
	        printError("Failed to read responses", errno);
	    }
        } else {
	    printError("Failed to setsockopt on response socket", errno);
	}
    } else {
        printError("Failed to bind to local port", errno);
    }
    return disposition;
}

enum Outcome sendHostnameBrodcastRequest(int sock)
{
    int value = 1; /* The value that the socket option will be set to. A value of one turns it on. */
    struct sockaddr_in bcastAddr;
    struct sockaddr *bcastAddrPtr;
    char *msg = "REPLY WITH HOSTNAME";
    enum Outcome stat = FAILURE; /* Assume failure until it has succeeded. */

    /* Configure it to broadcast to multiple receivers. */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &value, sizeof(value)) != SOCK_ERR) {
        initSubnetBroadcastAddress(&bcastAddr);
	bcastAddrPtr = (struct sockaddr *) &bcastAddr;
        /* Broadcast the data to the whole subnet.*/
	if (sendto(sock, msg, strlen(msg), 0, bcastAddrPtr, sizeof(bcastAddr)) != SOCK_ERR) {
            stat = SUCCESS;
        } else {
            printError("Failed to broadcast host name request", errno);
	}
    } else {
        printError("Failed to configure socket to broadcast", errno);
    }
    return stat;
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

void initReceiptAddress(struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(INADDR_ANY);
    address->sin_port = htons(CLIENT_PORT);
}
