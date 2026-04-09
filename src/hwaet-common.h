#ifndef HWAET_COMMON_H
#define HWAET_COMMON_H 1

#include <stdbool.h>    /* bool */
#include <sys/socket.h> /* socket, bind, getifaddrs, freeifaddrs, sockaddr, sa_family_t */
#include <arpa/inet.h>	/* sockaddr_in, in_port_t, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
#include <ifaddrs.h>	/* getifaddrs, freeifaddrs, ifaddrs */

/* Figure out if compiling under BSD and include the header for sockaddr_in. */
#if defined(__unix__)
  #include <sys/param.h>
  #if defined(BSD)
    #include <netinet/in.h> /* sockaddr_in */
  #endif
#endif

#define SA_PTR(a)    ((struct sockaddr *) a)
#define SA_IN_PTR(a) ((struct sockaddr_in *) a)

#define SHORT_STRING_SIZE 32

/**
 * RFC 1035 defines the maximum length of a fully qualified domain name to be
 * 255 characters.
 */
#define MAX_HOSTNAME_LEN 255

/**
 * Make this nonsense shorter.
 */
typedef struct sockaddr sa;

extern char *addressFamilyToString(sa_family_t family); /* Returns const char pointer */
extern char *addr2Str(struct sockaddr *addr);    /* Returns static char array  */
extern char *inetAddressToString(struct sockaddr_in *addr); /* Returns ptr to sys mem */
extern void printError(char *errorMessage, int errorNumber);
extern void handleError(char *msg, char *causalObject, int errnum);
extern void printInterface(struct ifaddrs *iface);
extern void dumpInterfaces();

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
