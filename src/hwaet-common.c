#include <stdio.h>
#include <string.h>	/* strerror */
#include <errno.h>	/* errno */
#include <net/if.h>	/* IFF_BROADCAST */
#include <arpa/inet.h>  /* inet_ntop */

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
    char *result;
    
    if (addr != NULL) {
        switch (addr->sa_family) {
        case AF_INET:
            result = ipAddr2Str((struct sockaddr_in *) addr);
            break;
        case AF_INET6:
            result = ip6Addr2Str((struct sockaddr_in6 *) addr)
            break;
        default:
            result = "\0";
            break;
        }
    }
    return result;
}

char *ipAddr2Str(struct sockaddr_in *addr)
{
    char addrText[INET4_ADDRSTRLEN+1];
    in_port_t port;
    static char addrAndPort[INET4_ADDRSTRLEN+7]; /* ipText + colon + port + terminator */
    
    /* TODO: Handle failures */
    inet_ntop(AF_INET, &(addr->sin_addr), addrText, sizeof(addrText));
    port = ntohs(addr->sin_port);
    snprintf(addrAndPort, sizeof(addrAndPort), "%s:%d", addrText, port);
    return addrAndPort;
}

char *ip6Addr2Str(struct sockaddr_in6 *addr)
{
    static char addrText[INET6_ADDRSTRLEN+1];
    
    inet_ntop(AF_INET6, &(addr->sin6_addr), addrText, sizeof(addrText));
    return addrText
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

