/* hwætd.c */

/* System headers */
#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset, strdup */
#include <signal.h>     /* signal, SIGCHLD, SIGHUP, SIG_IGN  */
#include <unistd.h>     /* close, POSIX gethostname */
#include <libgen.h>	/* basename */
#include <errno.h>	/* errno */
#include <syslog.h>     /* openlog, closelog */
#include <sys/stat.h>   /* umask */
#include <limits.h>     /* NAME_MAX */

#include "cross-platform-sockets.h"
#include "hwaet-common.h"
#include "string-ops.h"


void becomeDaemon();
char *getHostname();
bool getIpAddr(char *ipAddr, size_t capacity);
bool getHostIdentification(char *ident, size_t capacity);
bool runSocketServer(const char *hostname);
bool processRequests(const char *hostname, int svrSock, int cliSock);
void initReceiptAddress(struct sockaddr_in *address, in_port_t port);
bool isPrimaryInterface(struct ifaddrs *i);
bool isLoopback(struct sockaddr *address);
bool findPrimaryInterface(struct ifaddrs *i);


/* NAME_MAX is in bytes, not chars. This makes a difference with multibyte 
   encodings like UTF-8. NAME_MAX does not include the terminator byte. 
   So one byte needs to be added for that. */
char programName[NAME_MAX+1]; 
bool fatalErrorOccurred = false;

/* This needs to be removed. */
bool noErrors = true;

int main(int argc, char *argv[])
{
    char hostIdent[HOST_NAME_MAX+INET_ADDRSTRLEN+2]; /* Space and terminator */
    bool success = false;

    copyStr(programName, sizeof(programName), basename(argv[0]));
    becomeDaemon();
     
    openlog(programName, LOG_CONS | LOG_PID, LOG_DAEMON);

    if (getHostIdentification(hostIdent, sizeof(hostIdent))) {
        if (runSocketServer(hostIdent)) {
            success = true;
        }
    }
    closelog();
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

void becomeDaemon()
{
    pid_t pid;
    int fd;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0) {
        fprintf(stderr, "%s: Failed to fork: %s", programName, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0) {
        fprintf(stderr, "%s: Failed to create new session: %s", programName, strerror(errno));        
        exit(EXIT_FAILURE);
    }

    /* Catch, ignore and handle signals */
    /* TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0) {
       exit(EXIT_SUCCESS);
    }

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    for (fd = sysconf(_SC_OPEN_MAX); fd>=0; fd--) {
        close(fd);
    }
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
    return !fatalErrorOccurred;
}

bool getIpAddr(char *ipAddr, size_t capacity)
{
    struct ifaddrs ifPrime;
    
    if (findPrimaryInterface(&ifPrime)) {
        if (inet_ntop(AF_INET, &(((sain*)ifPrime.ifa_addr)->sin_addr), ipAddr, capacity) == NULL) {
            syslog(LOG_ERR, "Failed to get IP address of primary interface: %s: %m", ifPrime.ifa_name);
            fatalErrorOccurred = true;
        }
    }
    return !fatalErrorOccurred;
}

/**
 * Returns an internal statically allocated string. It will be empty if it
 * failed to get the hostname.
 */
char *getHostname()
{
    const int GETHOSTNAME_FAILURE=-1, GETHOSTNAME_SUCCESS=0;
    
    /* POSIX value, HOST_NAME_MAX, does not include the string terminator
       character, so 1 is added to length to allow room for the terminator. */
    static char hostname[HOST_NAME_MAX+1] = {'\0'}; /* Prevent garbage */
    size_t hnSz;
    char *result;
    
    hnSz = sizeof(hostname);

    if (gethostname(hostname, hnSz) == GETHOSTNAME_SUCCESS) {
        /* OpenBSD guarantees that the gethostname string parameter will end
           with a terminator character, but not all operating systems do. So, 
           the string terminator character will be added to make sure it 
           always works on every system. */
        hostname[hnSz-1] = '\0';
        result = hostname;
    } else {
        syslog(LOG_ERR, "Failed to get local hostname: %m");
        fatalErrorOccurred = true;
        result = NULL;
    }
    return result;
}


bool runSocketServer(const char *hostname)
{
    int svrSock;
    int cliSock;
    struct sockaddr_in svrAddr;
    bool success = false;

    if ((svrSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        syslog(LOG_INFO, "Created server socket with file descriptor: %d", svrSock);
        initReceiptAddress(&svrAddr, SERVER_PORT);
        if (bind(svrSock, (sa*) &svrAddr, sizeof(svrAddr)) != SOCK_ERR) {
            syslog(LOG_INFO, "Bound socket to port: %s", ipAddr2Str(&svrAddr));
            if ((cliSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
                syslog(LOG_INFO, "Created client socket with file descriptor: %d", cliSock);
                if (processRequests(hostname, svrSock, cliSock)) {
                    success = true;
                }
                close(cliSock);
            } else {
                syslog(LOG_ERR, "Failed to open client socket: %m");
            }
        } else {
            syslog(LOG_ERR, "Failed to bind to port: %s: %m", ipAddr2Str(&svrAddr));
        }
        close(svrSock);
    } else {
        syslog(LOG_ERR, "Failed to create server socket: %m");
    }
    return success;
}

bool processRequests(const char *hostname, int svrSock, int cliSock)
{
    /* cliAddr is large enough to handle any type of address. This is important
       because the recvfrom function's "from" parameter is an input/output
       parameter, so the client can send back any type of address, which could
       be many different sizes. */
    struct sockaddr_storage cliAddr;
    struct sockaddr_in *cliIpAddr;
    socklen_t cliAddrSz;
    char msg[32];
    size_t msgSz;
    size_t hnSz;
    bool success = true;

    cliAddrSz = sizeof(cliAddr);
    msgSz = sizeof(msg);
    hnSz = strlen(hostname) + 1;

    while (success) {
        syslog(LOG_INFO, "Waiting for requests");
        if (recvfrom(svrSock, msg, msgSz, 0, (sa*) &cliAddr, &cliAddrSz) != SOCK_ERR) {
            if (cliAddr.ss_family == AF_INET) {
                /* cliAddr is the right type, so continue. */
                cliIpAddr = (struct sockaddr_in *) &cliAddr; /* Cast once for convenience */
                syslog(LOG_INFO, "Received message from: %s", ipAddr2Str(cliIpAddr));
                cliIpAddr->sin_port = htons(CLIENT_PORT);
                syslog(LOG_INFO, "Changed port in address: %s", ipAddr2Str(cliIpAddr));
                if (sendto(cliSock, hostname, hnSz, 0, (sa*) cliIpAddr, cliAddrSz) != SOCK_ERR) {
                    syslog(LOG_INFO, "Sent hostname %s to %s", hostname, ipAddr2Str(cliIpAddr));
                } else {
                    syslog(LOG_ERR, "Failed to send hostname to client: %s: %m", ipAddr2Str(cliIpAddr));
                }
            } else {
                /* This code cannot yet process connections from a non-IPv4 
                   client because it can't create an address and port
                   for other types of addresses. */
                syslog(LOG_INFO, "Client is using a non-IPv4 address family: %d", cliAddr.ss_family);
                /* This is not a fatal error because it is just one client. */
            }
        } else {
            syslog(LOG_ERR, "Failed to setup hostname receiver: %m");
            success = false;
        }
    }
    return success;
}

void initReceiptAddress(struct sockaddr_in *address, in_port_t port)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(INADDR_ANY);
    address->sin_port = htons(port);
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

bool findPrimaryInterface(struct ifaddrs *primary)
{
    struct ifaddrs *ifaceList; /* Required for freeifaddrs */
    struct ifaddrs *i;
    bool found = false;
    
    if (getifaddrs(&ifaceList) != SOCK_ERR) {
	for (i = ifaceList; !found && i!=NULL; i = i->ifa_next) {
            if (isPrimaryInterface(i)) {
	    	*primary = *i; /* Copy whole struct from system to result. */
		found = true;
            }
	}
	freeifaddrs(ifaceList); /* Cannot return before this is free'd. */
	if (!found) {
            syslog(LOG_ERR, "Search for primary interface failed to find one");
	}
    } else {
        syslog(LOG_ERR, "Failed to get network interfaces: %m");
    }
    fatalErrorOccurred = !found;
    return found;
}
