#include <stdio.h>
#include "hwaet-common.h"

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

char *inetAddressToString(struct sockaddr_in *addr)
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
