#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset */
#include <unistd.h>     /* close */
#include <libgen.h>	/* basename */
#include <errno.h>	/* errno */

#include <sys/types.h>	/* getifaddrs, freeifaddrs */
#include <sys/socket.h> /* socket, bind, getifaddrs, freeifaddrs */
#include <arpa/inet.h>	/* sockaddr_in, in_port_t, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
#include <ifaddrs.h>	/* getifaddrs, freeifaddrs */
#include <net/if.h>	/* IFF_BROADCAST */

/* Figure out if compiling under BSD and include the header for sockaddr_in. */
#if defined(__unix__)
  #include <sys/param.h>
  #if defined(BSD)
    #include <netinet/in.h> /* sockaddr_in */
  #endif
#endif

#define SAPTR(a)   ((struct sockaddr *) a)
#define SAINPTR(a) ((struct sockaddr_in *) a)

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
enum CompletionType sendHostnameBrodcastRequest(int socket);
enum CompletionType readResponses(int socket);
void printError(char *errorMessage, int errorNumber);
char *sockaddrinToString(struct sockaddr_in *addr);
void dumpInterfaces();


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

enum CompletionType sendHostnameBrodcastRequest(int sock)
{
    int value = 1; /* The value that the socket option will be set to. A value of one turns it on. */
    struct sockaddr_in bcastAddr;
    struct sockaddr *bcastAddrPtr;
    char *msg = "REPLY WITH HOSTNAME";
    enum CompletionType stat = FAILURE; /* Assume failure until it has succeeded. */

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

char *addressToString(struct sockaddr *addr)
{
    /* Only this one buffer is needed because it is bigger than the IPV4 buffer. */
    static char addrTextBuf[INET6_ADDRSTRLEN] = {'\0'};
    struct sockaddr_in *addrIPv4;
    struct sockaddr_in6 *addrIPv6;

    if (addr != NULL) {
        switch (addr->sa_family) {
        case AF_INET:
	    addrIPv4 = (struct sockaddr_in *) addr;
	    inet_ntop(AF_INET, &(addrIPv4->sin_addr), addrTextBuf, INET_ADDRSTRLEN);
            break;
	case AF_INET6:
	    addrIPv6 = (struct sockaddr_in6 *) addr;
	    inet_ntop(AF_INET6, &(addrIPv6->sin6_addr), addrTextBuf, INET6_ADDRSTRLEN);
	    break;
        default:
            addrTextBuf[0] = '\0';
            break;
        }
    }
    return (char *) &addrTextBuf;
}

char *sockaddrinToString(struct sockaddr_in *addr)
{
	char *addrText = "";
	struct sockaddr *sa;
	
	if (addr != NULL) {
		sa = (struct sockaddr *) addr;
		if (sa->sa_family == AF_INET) {
			addrText = inet_ntoa((addr)->sin_addr);
		} else {
			addrText = "Not an AF_INET address";
		}
	} else {
		addrText = "No socket address given";
	}
	return addrText;
}

/*
void printAddress(const char *name, struct sockaddr *address)
{
    if (address != NULL) {
        printf("%s=%s", name, addressToString(address));
    }
}
*/

void printInterface(struct ifaddrs *iface)
{
	if (iface != NULL) {
		/* The buffer in addressToString is static so calls to it must be in 
		   separate printfs or all the values will be the value of the last call. */
		printf("name=%s family=%s addr=%s ",
			iface->ifa_name,
			addressFamilyToString(iface->ifa_addr->sa_family),
			addressToString(iface->ifa_addr));
		printf("netmask=%s ", addressToString(iface->ifa_netmask));
		printf("broadaddr=%s ", addressToString(iface->ifa_broadaddr));
		printf("dstaddr=%s\n", addressToString(iface->ifa_dstaddr));
	}
}

void dumpInterfaces()
{
    struct ifaddrs *list;
    struct ifaddrs *node;

    if (getifaddrs(&list) != SOCK_ERR) {
	node = list;
	while (node != NULL) {
	    printInterface(node);
            /* Advance */
            node = node->ifa_next;
	}
	freeifaddrs(list);
    } else {
        printError("Failed to get network interfaces", errno);
    }
}
