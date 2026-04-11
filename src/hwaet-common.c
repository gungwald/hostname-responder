#include <stdio.h>
#include <string.h>	/* strerror */
#include <errno.h>	/* errno */
#include <net/if.h>	/* IFF_BROADCAST */

#include "hwaet-common.h"

const int SOCK_ERR = -1;
const in_port_t SERVER_PORT = 4140;
const in_port_t CLIENT_PORT = 4141;

char *programName = NULL;
bool noErrors = true;


void printError(char *errorMessage, int errorNumber)
{
    fprintf(stderr, "%s: %s: %s\n", programName, errorMessage, strerror(errorNumber));
}

void handleError(char *msg, char *causalObject, int errNum)
{
    if (errNum == 0 && causalObject == NULL)
        fprintf(stderr, "%s: %s\n", programName, msg);
    else if (errNum == 0)
        fprintf(stderr, "%s: %s: %s\n", programName, msg, causalObject);
    else if (causalObject == NULL)
        fprintf(stderr, "%s: %s: %s\n", programName, msg, strerror(errNum));
    else
        fprintf(stderr, "%s: %s: %s: %s\n", programName, msg, causalObject, strerror(errNum));
    noErrors = false;
}


char *addrFam2Str(sa_family_t family)
{
    static char unkFamNum[8];
    char *name;

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
        snprintf(unkFamNum, sizeof(unkFamNum), "%d", family);
        name = unkFamNum;
        break;
    }
    return name;
}

char *addr2Str(struct sockaddr *addr)
{
    char ipText[INET6_ADDRSTRLEN+1]; /* Works for IP v4 and v6 */
    in_port_t port;
    static char result[INET6_ADDRSTRLEN+7]; /* ipText + colon + port + terminator */
    
    if (addr != NULL) {
        switch (addr->sa_family) {
        case AF_INET:
            /* TODO: Handle failures */
            inet_ntop(AF_INET, &(((sain*)addr)->sin_addr), ipText, sizeof(ipText));
	    port = ntohs(((sain *) addr)->sin_port);
	    snprintf(result, sizeof(result), "%s:%d", ipText, port);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((sain6*)addr)->sin6_addr), ipText, sizeof(ipText));
	    snprintf(result, sizeof(result), "%s", ipText);
            break;
        default:
            result[0] = '\0';
            break;
        }
    }
    return result;
}


void printInterface(struct ifaddrs *iface)
{
    if (iface != NULL) {
        /* The buffer in addressToString is static so calls to it must be in
           separate printfs or all the values will be the value of the last call. */
        printf("name=%s family=%s addr=%s ",
               iface->ifa_name,
               addrFam2Str(iface->ifa_addr->sa_family),
               addr2Str(iface->ifa_addr));
        printf("netmask=%s ", addr2Str(iface->ifa_netmask));
        printf("broadaddr=%s ", addr2Str(iface->ifa_broadaddr));
        printf("dstaddr=%s\n", addr2Str(iface->ifa_dstaddr));
    }
}

void printInterfaces()
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
        handleError("Failed to get network interfaces", NULL, errno);
    }
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
