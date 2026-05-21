#ifndef HWAET_COMMON_H
#define HWAET_COMMON_H 1

#include <stdbool.h>			/* bool */
#include "cross-platform-sockets.h"


/**
 * Port on which the hostname request is sent by the client.
 */
#define SERVER_PORT ((in_port_t) 4140)

/**
 * Port on which the response is returned from the server to the client.
 */
#define CLIENT_PORT ((in_port_t) 4141)

extern char *addrFam2Str(sa_family_t family);     /* Returns const char pointer */
extern char *addr2Str(struct sockaddr *addr);     /* Returns static char array  */
extern char *ipAddr2Str(struct sockaddr_in *addr);
extern char *ip6Addr2Str(struct sockaddr_in6 *addr);
extern void printError(char *errorMessage, int errorNumber);
extern void handleError(char *msg, char *causalObject, int errnum);
extern void printInterface(struct ifaddrs *iface);
extern void printInterfaces();
bool findBroadcastAddr(struct sockaddr_in *addr);


#endif
