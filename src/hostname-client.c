#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset */
#include <unistd.h>     /* close */
#include <libgen.h>	/* basename */
#include <errno.h>	/* errno */

#include <sys/types.h>	/* getifaddrs, freeifaddrs */
#include <sys/socket.h> /* socket, bind, getifaddrs, freeifaddrs */
#include <arpa/inet.h>	/* sockaddr_in, in_port_t */
#if defined(__unix__)
  #include <sys/param.h>
  #if defined(BSD)
    #include <netinet/in.h> /* sockaddr_in */
  #endif
#endif
#include <ifaddrs.h>	/* getifaddrs, freeifaddrs */
#include <net/if.h>	/* IFF_BROADCAST */

#define SHORT_STRING_SIZE 32

/**
 * RFC 1035 defines the maximum length of a fully qualified domain name to be
 * 255 characters.
 */
#define MAX_HOSTNAME_LEN 255

/* How a function completed. */
enum CompletionType {FAILURE=0, SUCCESS=1};
/* All socket functions return this when they fail. */
const int SOCK_ERR = -1;
const in_port_t REQUEST_PORT = 4140;
const in_port_t RESPONSE_PORT = 4141;

bool isLoopback(struct sockaddr *address);
bool isPrimaryInterface(struct ifaddrs *i);
struct sockaddr_in *findIPv4Broadcast();
void initSubnetBroadcastAddress(struct sockaddr_in *address);
void initReceiptAddress(struct sockaddr_in *address);
enum CompletionType sendHostnameRequest(int socket);
enum CompletionType readResponses(int socket);
void printError(char *errorMessage, int errorNumber);



char *programName = NULL;


int main(int argc, char *argv[])
{
    int sock;
    int exitStatus = EXIT_FAILURE;

    programName = basename(argv[0]);
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        if (sendHostnameRequest(sock)) {
            if (readResponses(sock)) {
                exitStatus = EXIT_SUCCESS;
            }
        }
        close(sock);
    } else {
        printError("Failed to create socket", errno);
    }
    return exitStatus;
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
    static struct sockaddr_in bcastAddress;
    struct ifaddrs *ifaceList; /* Required for freeifaddrs */
    struct ifaddrs *iface;
    struct sockaddr_in *bcastAddrSysPtr;

    if (getifaddrs(&ifaceList) != SOCK_ERR) {
	for (iface = ifaceList; iface != NULL; iface = iface->ifa_next) {
            if (isPrimaryInterface(iface)) {
        	bcastAddrSysPtr = (struct sockaddr_in *) iface->ifa_broadaddr;
        	/* Copy the whole broadcast address structure into static variable. */
        	bcastAddress = *bcastAddrSysPtr;
		/* The broadcast address was found, so exit the loop. */
        	break;
            }
	}
	freeifaddrs(ifaceList);
    } else {
        printError("Failed to get network interfaces", errno);
    }
    return &bcastAddress;
}

enum CompletionType readResponses(int sock)
{
    char resp[MAX_HOSTNAME_LEN+1];
    struct sockaddr from;
    socklen_t fromLen;
    int byteCount;
    struct sockaddr_in receiptAddr;
    enum CompletionType disposition = FAILURE;
    struct timeval timeout = {.tv_sec=5, .tv_usec=0};

    initReceiptAddress(&receiptAddr);
    if (bind(sock, (struct sockaddr *) &receiptAddr, sizeof(receiptAddr)) != SOCK_ERR) {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != SOCK_ERR) {
            while ((byteCount = recvfrom(sock, resp, MAX_HOSTNAME_LEN, 0, &from, &fromLen)) != SOCK_ERR && byteCount > 0) {
                puts(resp);
            }
            byteCount==0 ? disposition=SUCCESS : printError("Failed to receive host name", errno);
        } else {
	    printError("Failed to setsockopt", errno);
	}
    } else {
        printError("Failed to bind to local port", errno);
    }
    return disposition;
}

enum CompletionType sendHostnameRequest(int sock)
{
    int optVal = 1;
    struct sockaddr_in bcastAddr;
    struct sockaddr *bcastPtr;
    char *msg = "REPLY WITH HOSTNAME";
    enum CompletionType cStat = FAILURE;

    /* Configure it to broadcast to multiple receivers. */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optVal, sizeof(optVal)) != SOCK_ERR) {
        initSubnetBroadcastAddress(&bcastAddr);
	bcastPtr = (struct sockaddr *) &bcastAddr;
        /* Broadcast the data to the whole subnet.*/
	if (sendto(sock, msg, strlen(msg), 0, bcastPtr, sizeof(bcastAddr)) != SOCK_ERR) {
            cStat = SUCCESS;
        } else {
            printError("Failed to broadcast host name request", errno);
	}
    } else {
        printError("Failed to configure socket to broadcast", errno);
    }
    return cStat;
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
    address->sin_port = htons(REQUEST_PORT);
}

void initReceiptAddress(struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(INADDR_ANY);
    address->sin_port = htons(RESPONSE_PORT);
}

void printError(char *errorMessage, int errorNumber)
{
    fprintf(stderr, "%s: %s: %s\n", programName, errorMessage, strerror(errorNumber));
}

/* Debug functions */
char *addressFamilyToString(sa_family_t family)
{
    char *name;
    static char unknownNumber[SHORT_STRING_SIZE];

    switch (family) {
    case AF_APPLETALK:
        name = "AF_APPLETALK";
        break;
    case AF_CCITT:
        name = "AF_CCITT";
        break;
    case AF_CHAOS:
        name = "AF_CHAOS";
        break;
    case AF_INET:
        name = "AF_INET";
        break;
    case AF_INET6:
        name = "AF_INET6";
        break;
    case AF_IPX:
        name = "AF_IPX";
        break;
    case AF_ISDN:
        name = "AF_ISDN";
        break;
    case AF_ISO:
        name = "AF_ISO";
        break;
    case AF_LINK:
        name = "AF_LINK";
        break;
#if defined(__linux__)
    case AF_NETBIOS:
        name = "AF_NETBIOS";
        break;
    case AF_SYSTEM:
        name = "AF_SYSTEM";
        break;
#endif
    case AF_UNIX:
        name = "AF_UNIX";
        break;
    default:
        snprintf(unknownNumber, sizeof(unknownNumber), "%d", family);
        name = unknownNumber;
        break;
    }
    return name;
}

char *addressToString(struct sockaddr *address)
{
    char *addrText;

    if (address != NULL) {
        switch (address->sa_family) {
        case AF_INET:
            addrText = inet_ntoa(((struct sockaddr_in *) address)->sin_addr);
            break;
        default:
            addrText = "";
            break;
        }
    } else {
        addrText = "NULL";
    }
    return addrText;
}

void printAddress(const char *name, struct sockaddr *address)
{
    if (address != NULL) {
        printf("%s=%s", name, addressToString(address));
    }
}

void dumpInterfaces()
{
    struct ifaddrs *list;
    struct ifaddrs *node;

    if (getifaddrs(&list) == SOCK_ERR) {
        printError("Failed to get network interfaces", errno);
    } else {
	node = list;
	while (node != NULL) {
            printf("name=%s", node->ifa_name);
            if (node->ifa_addr != NULL) {
        	printf(" family=%s", addressFamilyToString(node->ifa_addr->sa_family));
        	printAddress(" address", node->ifa_addr);
        	printAddress(" netmask", node->ifa_netmask);
        	printAddress(" broadcast", node->ifa_dstaddr);
            }
            printf("\n");
            /* Advance */
            node = node->ifa_next;
	}
	freeifaddrs(list);
    }
}

