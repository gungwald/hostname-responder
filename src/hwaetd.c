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

char *getLocalHostName();
bool isEmpty(const char *s);
enum Outcome processRequests(const char *hostname);
void initReceiptAddress(struct sockaddr_in *address);
enum Outcome sendHostname(struct sockaddr_in *callerAddr, const char *hostname);

char *programName = NULL;
bool noErrors = true;


int main(int argc, char *argv[])
{
    int exitCode = EXIT_FAILURE;
    char *hostname;
    
    programName = basename(argv[0]);
    
    hostname = getLocalHostName();
    if (noErrors) {
        processRequests(hostname);
    	if (noErrors) {
            exitCode = EXIT_SUCCESS;
        }
    }
    return exitCode;
}

    

/**
 * Returns an internal statically allocated string. It will be empty if it
 * failed to get the hostname.
 */
char *getLocalHostName()
{
    /* POSIX value, HOST_NAME_MAX, does not include the string terminator 
       character, so 1 is added to length to allow room for the terminator. */
    static char hostname[HOST_NAME_MAX+1] = {'\0'}; /* Prevent garbage */

    if (gethostname(hostname, HOST_NAME_MAX+1) == GHN_SUCCESS) {
        /* OpenBSD guarantees '\0' termination, but not all operating systems do. */
        hostname[HOST_NAME_MAX] = '\0';
    } else {
        hostname[0] = '\0'; /* Prevent failures by avoiding an undefined state. */
        printError("Failed to get hostname", errno);
        noErrors = false;
    }
    return hostname;
}

bool isEmpty(const char *s)
{
    return s==NULL || s[0]=='\0';
}

int createSocket()
{
    int sock;
        
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == SOCK_ERR)
        printError("Failed to create socket", errno);
    return sock;
}

enum Outcome processRequests(const char *hostname)
{
    enum Outcome outcome = FAILURE; /* All operations must succeed to achieve success. */
    int recvSock;
    struct sockaddr_in *callerAddr;

    if ((recvSock = createSocket()) != SOCK_ERR) {
        if (bindToPort(recvSock, SERVER_PORT)) {
	    outcome = SUCCESS; /* Prepare for while loop. */
	    while (outcome == SUCCESS) {
	        outcome = FAILURE; /* Each iteration must achieve it's own success. */
	        if ((callerAddr = recvHostnameReq(recvSock)) != NULL) {
		    if (sendHostname(callerAddr, hostname)) {
		        outcome = SUCCESS;
		    }
		}
            }
        }
	close(recvSock);	
    }
    return outcome;
}

struct sockaddr_in *recvHostnameReq(int sock)
{
    char msg[32];
    size_t msgSize;
    static struct sockaddr_in callerAddr;
    socklen_t callerAddrSize;
    struct sockaddr_in *resultCallerAddr;
    
    msgSize = sizeof(msg);
    fromAddrSize = sizeof(callerAddr);
    if (recvfrom(sock, msg, msgSize, 0, &callerAddr, &callerAddrSize) != SOCK_ERR) {
        resultCallerAddr = &callerAddr;
    } else {
        resultCallerAddr = NULL;
        printError("Failed to receive hostname request from any caller", errno);
    }
    return resultCallerAddr;
}

enum Outcome bindToPort(int sock, in_port_t port)
{
    enum Outcome outcome;
    struct sockaddr_in recvAddr;

    initReceiptAddress(&recvAddr, port);
    if (bind(sock, (struct sockaddr *) &recvAddr, sizeof(recvAddr)) != SOCK_ERR) {
        outcome = SUCCESS;
    } else {
        outcome = FAILURE;
        printError("Failed to bind to port", errno);
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

