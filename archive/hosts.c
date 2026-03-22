#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <sys/types.h>	/* getifaddrs, freeifaddrs */
#include <sys/socket.h> /* socket, bind, getifaddrs, freeifaddrs */
#include <arpa/inet.h>  /* sockaddr_in */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset */
#include <unistd.h>     /* close */
#include <ifaddrs.h>	/* getifaddrs, freeifaddrs */
#include <errno.h>		/* errno */
#include <net/if.h>		/* IFF_BROADCAST */

bool isLoopback(struct sockaddr *address);
bool isPrimaryInterface(struct ifaddrs *i);
struct sockaddr_in *findIPv4Broadcast();
void buildSubnetBroadcastAddress(struct sockaddr_in *address);
void buildResponseReceiptAddress(struct sockaddr_in *address);
enum Status broadcastHostnameRequest(int socket);
enum Status readResponses(int udpSocket);
void fail(char *errorMessage, int errorNumber);
void printError(char *errorMessage, int errorNumber);

#define SHORT_STRING_SIZE 32

/**
 * RFC 1035 defines the maximum length of a fully qualified domain name to be
 * 255 characters.
 */
#define MAX_HOSTNAME_LEN 255

enum Status {FAILURE, SUCCESS};
const int SOCK_API_ERR = -1;
const in_port_t REQUEST_PORT = 4140;
const in_port_t RESPONSE_PORT = 4141;

char *programName;

int main(int argc, char *argv[])
{
    int udpSocket;
    int exitStatus = EXIT_FAILURE;

    if ((udpSocket = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) != SOCK_API_ERR) {
        if (broadcastHostnameRequest(udpSocket)) {
        	if (readResponses(udpSocket))
                exitStatus = EXIT_SUCCESS;
        }
        close(udpSocket);
    }
    else {
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

bool isPrimaryInterface(struct ifaddrs *i)
{
    return i->ifa_addr != NULL
    		&& i->ifa_addr->sa_family == AF_INET
    		&& (i->ifa_flags & IFF_BROADCAST)
    		&& !isLoopback(i->ifa_addr);
}

struct sockaddr_in *findIPv4Broadcast()
{
    static struct sockaddr_in broadcastAddress;
    struct ifaddrs *list;
    struct ifaddrs *i;
    struct sockaddr_in *broadcastAddressPointer;

    if (getifaddrs(&list) == SOCK_API_ERR)
    	fail("Failed to get network interfaces", errno);

    for (i = list; i != NULL; i = i->ifa_next) {
        if (isPrimaryInterface(i)) {
            broadcastAddressPointer = (struct sockaddr_in *) i->ifa_dstaddr;
            // Copy the broadcast address data into local static memory.
            broadcastAddress = *broadcastAddressPointer;
        	break;
        }
    }
    freeifaddrs(list);
    return &broadcastAddress;
}

enum Status readResponses(int udpSocket)
{
    char response[MAX_HOSTNAME_LEN+1];
    struct sockaddr fromAddr;
    socklen_t fromAddrLen;
    int numBytesRecvd;
    struct sockaddr_in recptAddr;
    enum Status status = FAILURE;
    struct timeval timeout;

    buildResponseReceiptAddress(&recptAddr);
    if (bind(udpSocket, (struct sockaddr *) &recptAddr, sizeof(recptAddr)) != SOCK_API_ERR) {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
    	if (setsockopt(udpSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)) != SOCK_API_ERR) {
        	while ((numBytesRecvd = recvfrom(udpSocket, response, MAX_HOSTNAME_LEN, 0, &fromAddr, &fromAddrLen)) != SOCK_API_ERR && numBytesRecvd > 0) {
                puts(response);
        	}
        	if (numBytesRecvd == 0)
        		status = SUCCESS;
        	else
        		printError("Failed to receive host name", errno);
    	}
    }
    else
    	printError("Failed to bind to local port", errno);

    return status;
}

enum Status broadcastHostnameRequest(int udpSocket)
{
    int on = 1;
    struct sockaddr_in bcastAddr;
    char *data = "What is your name?";
    enum Status status = FAILURE;

    /* Configure it to broadcast to multiple receivers. */
    if (setsockopt(udpSocket,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on)) != SOCK_API_ERR) {
        buildSubnetBroadcastAddress(&bcastAddr);
        /* Broadcast the data to the whole subnet.*/
        if (sendto(udpSocket,data,strlen(data),0,(struct sockaddr*)&bcastAddr,sizeof(bcastAddr)) != SOCK_API_ERR)
            status = SUCCESS;
        else
        	printError("Failed to broadcast host name request", errno);
    }
    else {
    	printError("Failed to configure socket to broadcast", errno);
    }
    return status;
}

/**
 * Build the address for the intended receivers, which are all hosts on this
 * subnet.
 */
void buildSubnetBroadcastAddress(struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(findIPv4Broadcast()->sin_addr.s_addr);
    address->sin_port = htons(REQUEST_PORT);
}

void buildResponseReceiptAddress(struct sockaddr_in *address)
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

void fail(char *errorMessage, int errorNumber)
{
    printError(errorMessage, errorNumber);
    exit(EXIT_FAILURE);
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
	case AF_NETBIOS:
		name = "AF_NETBIOS";
		break;
	case AF_SYSTEM:
		name = "AF_SYSTEM";
		break;
	case AF_UNIX:
		name = "AF_UNIX";
		break;
	default:
        sprintf(unknownNumber, "%d", family);
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
    }
    else {
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

    if (getifaddrs(&list) == SOCK_API_ERR) {
    	fail("Failed to get network interfaces", errno);
    }

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

