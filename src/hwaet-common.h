#ifndef HWAET_COMMON_H
#define HWAET_COMMON_H 1

#include <stdbool.h>			/* bool */
#include "cross-platform-sockets.h"

/**
 * The "struct sockaddr" type has to be used in many casts. Defining it to
 * something shorter is very helpful.
 */
typedef struct sockaddr sa;         /* type sa    = struct sockaddr     */
typedef struct sockaddr_in sain;    /* type sain  = struct sockaddr_in  */
typedef struct sockaddr_in6 sain6;  /* type sain6 = struct sockaddr_in6 */

extern char *addrFam2Str(sa_family_t family);     /* Returns const char pointer */
extern char *addr2Str(struct sockaddr *addr);     /* Returns static char array  */
extern char *ipAddr2Str(struct sockaddr_in *addr);
extern char *ip6Addr2Str(struct sockaddr_in6 *addr);
extern void printError(char *errorMessage, int errorNumber);
extern void handleError(char *msg, char *causalObject, int errnum);
extern void printInterface(struct ifaddrs *iface);
extern void printInterfaces();
bool findBroadcastAddr(struct sockaddr_in *addr);

/**
 * The sentinel value all socket functions return this when they fail.
 */
extern const int SOCK_ERR;

/**
 * Port on which the hostname request is sent by the client.
 */
extern const in_port_t SERVER_PORT;

/**
 * Port on which the response is returned from the server to the client.
 */
extern const in_port_t CLIENT_PORT;

extern char *programName;
extern bool noErrors;

#endif
