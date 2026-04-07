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
void processRequests(const char *hostname);
void initReceiptAddress(struct sockaddr_in *address);

char *programName = NULL;
bool noErrors = true;


int main(int argc, char *argv[])
{
    char *hostname;

    programName = basename(argv[0]);

    if ((hostname = getHostname()) != NULL) {
        processRequests(hostname);
    }
    return noErrors ? EXIT_SUCCESS : EXIT_FAILURE;
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
    char *result;

    if (gethostname(hostname, HOST_NAME_MAX+1) == GHN_SUCCESS) {
        /* OpenBSD guarantees that the gethostname string parameter will end
           with a terminator character, but not all operating systems do. So, 
           the string terminator character will be added to make sure it 
           always works on every system. */
        hostname[HOST_NAME_MAX] = '\0';
        result = hostname;
    } else {
        hostname[0] = '\0'; /* Prevent failures by avoiding an undefined state. */
        printError("Failed to get hostname", errno);
        noErrors = false;
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
    size_t hnSiz;
    typedef struct sockaddr sa;

    addrSz = sizeof(struct sockaddr_in);
    msgSz = sizeof(msg);
    hnSz = strlen(hostname) + 1;

    if ((svrSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        initReceiptAddress(&svrAddr, SERVER_PORT);
        if (bind(svrSock, (sa*) &svrAddr, addrSz) != SOCK_ERR) {
            if (cliSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
                while (noErrors) {
                    if (recvfrom(svrSock, msg, msgSz, 0, (sa*) &cliAddr, &addrSz) != SOCK_ERR) {
                        cliAddr.sin_port = htons(CLIENT_PORT);
                        if (sendto(cliSock, hostname, hnSz, 0, (sa*) &cliAddr, addrSz) != SOCK_ERR) {

                        } else {
                            printError("Failed to send hostname to client", errno);
                            noErrors = false;
                        }
                    } else {
                        printError("Failed to receive hostname request from any caller", errno);
                        noErrors = false;
                    }
                }
                close(cliSock);
            } else {
                printError("Failed to open client socket", errno);
                noErrors = false;
            }
        } else {
            printError("Failed to bind to port", errno);
            noErrors = false;
        }
        close(svrSock);
    } else {
        printError("Failed to create socket", errno);
        noErrors = false;
    }
    return outcome;
}

void initReceiptAddress(struct sockaddr_in *address, in_port_t port)
{
    memset(address, 0, sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(INADDR_ANY);
    address->sin_port = htons(port);
}

