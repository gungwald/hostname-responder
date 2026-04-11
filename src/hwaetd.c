/* hwætd.c */
#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset */
#include <unistd.h>     /* close, POSIX gethostname */
#include <libgen.h>	/* basename */
#include <errno.h>	/* errno */

#include <sys/types.h>	/* getifaddrs, freeifaddrs */
#include <sys/socket.h> /* socket, bind, getifaddrs, freeifaddrs */
#include <arpa/inet.h>	/* sockaddr_in, in_port_t, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
#include <ifaddrs.h>	/* getifaddrs, freeifaddrs */
#include <net/if.h>	/* IFF_BROADCAST */
#include <limits.h>	/* POSIX HOST_NAME_MAX */

/* Figure out if compiling under BSD and include the header for sockaddr_in. */
#if defined(__unix__)
#include <sys/param.h>
#if defined(BSD)
#include <netinet/in.h> /* sockaddr_in */
#endif
#endif

#include "hwaet-common.h"

enum GetHostnameOutcome {GHN_FAILURE=-1, GHN_SUCCESS=0};

char *getHostname();
bool getIpAddr(char *ipAddr, size_t capacity);
bool getHostIdentification(char *ident, size_t capacity);
void processRequests(const char *hostname);
void initReceiptAddress(struct sockaddr_in *address, in_port_t port);


int main(int argc, char *argv[])
{
    char hostIdent[HOST_NAME_MAX+INET_ADDRSTRLEN+2]; /* Space and terminator */

    programName = basename(argv[0]);
    noErrors = true;

    if (getHostIdentification(hostIdent, sizeof(hostIdent))) {
        processRequests(hostIdent);
    }
    return noErrors ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool getHostIdentification(char *ident, size_t capacity)
{
    char *hostname;
    char ipAddr[INET_ADDRSTRLEN]; /* Long enough for terminator on OpenBSD */
    
    if ((hostname = getHostname()) != NULL) {
        if (getIpAddr(ipAddr, sizeof(ipAddr))) {
            snprintf(ident, capacity, "%s %s", hostname, ipAddr);
        }
    }
    return noErrors;
}

bool getIpAddr(char *ipAddr, size_t capacity) {
    struct ifaddrs ifPrime;
    
    if (findPrimaryInterface(&ifPrime)) {
        if (inet_ntop(AF_INET, &(((sain*)ifPrime.ifa_addr)->sin_addr), ipAddr, capacity) == NULL) {
            handleError("Failed to get IP address of primary interface", ifPrime.ifa_name, errno);
        }
    }
    return noErrors;
}

/**
 * Returns an internal statically allocated string. It will be empty if it
 * failed to get the hostname.
 */
char *getHostname()
{
    /* POSIX value, HOST_NAME_MAX, does not include the string terminator
       character, so 1 is added to length to allow room for the terminator. */
    static char hostname[HOST_NAME_MAX+1] = {'\0'}; /* Prevent garbage */
    size_t hnSz;
    char *result;
    
    hnSz = sizeof(hostname);

    if (gethostname(hostname, hnSz) == GHN_SUCCESS) {
        /* OpenBSD guarantees that the gethostname string parameter will end
           with a terminator character, but not all operating systems do. So, 
           the string terminator character will be added to make sure it 
           always works on every system. */
        hostname[hnSz-1] = '\0';
        result = hostname;
    } else {
        hostname[0] = '\0'; /* Prevent failures by avoiding an undefined state. */
        handleError("Failed to get hostname", NULL, errno);
        result = NULL;
    }
    return result;
}


void processRequests(const char *hostname)
{
    int svrSock;
    int cliSock;
    struct sockaddr_in svrAddr;
    struct sockaddr_in cliAddr;
    socklen_t addrSz;
    char msg[32];
    size_t msgSz;
    size_t hnSz;

    addrSz = sizeof(struct sockaddr_in);
    msgSz = sizeof(msg);
    hnSz = strlen(hostname) + 1;

    if ((svrSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        printf("%s: Created server socket with file descriptor: %d\n", programName, svrSock);
        initReceiptAddress(&svrAddr, SERVER_PORT);
        if (bind(svrSock, (sa*) &svrAddr, addrSz) != SOCK_ERR) {
            printf("%s: Bound socket to port: %s\n", programName, addr2Str((sa*)&svrAddr));
            if ((cliSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
                while (noErrors) {
                    printf("%s: Waiting for requests...\n", programName);
                    if (recvfrom(svrSock, msg, msgSz, 0, (sa*) &cliAddr, &addrSz) != SOCK_ERR) {
                        printf("%s: Received message from: %s\n", programName, addr2Str((sa*)&cliAddr));
                        cliAddr.sin_port = htons(CLIENT_PORT);
                        printf("%s: Changed port in address: %s\n", programName, addr2Str((sa*)&cliAddr));
                        if (sendto(cliSock, hostname, hnSz, 0, (sa*) &cliAddr, addrSz) != SOCK_ERR) {
                            printf("%s: Sent hostname %s to %s\n", programName, hostname, addr2Str((sa*)&cliAddr));
                        } else {
                            handleError("Failed to send hostname to client", addr2Str((sa*)&cliAddr), errno);
                        }
                    } else {
                        handleError("Failed to receive hostname request from any caller", NULL, errno);
                    }
                }
                close(cliSock);
            } else {
                handleError("Failed to open client socket", NULL, errno);
            }
        } else {
            handleError("Failed to bind to port", addr2Str((sa*)&svrAddr), errno);
        }
        close(svrSock);
    } else {
        handleError("Failed to create server socket", NULL, errno);
    }
}

void initReceiptAddress(struct sockaddr_in *address, in_port_t port)
{
    memset(address, 0, sizeof(address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(INADDR_ANY);
    address->sin_port = htons(port);
}

