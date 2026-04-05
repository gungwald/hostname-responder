#ifndef HWÆT_COMMON_H
#define HWÆT_COMMON_H 1

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

#define SHORT_STRING_CAPACITY 32

/**
 * RFC 1035 defines the maximum length of a fully qualified domain name to be
 * 255 characters.
 */
#define MAX_HOSTNAME_LEN 255

/**
 * Reports whether a function was successful or whether it failed.
 */
enum Outcome {FAILURE=0, SUCCESS=1};

/**
 * The sentinel value all socket functions return this when they fail.
 */
const int SOCK_ERR = -1;

/**
 * Port on which the hostname request is sent by the client.
 */
const in_port_t SERVER_PORT = 4140;

/**
 * Port on which the response is returned from the server to the client.
 */
const in_port_t CLIENT_PORT = 4141;

extern char *addressFamilyToString(sa_family_t family); /* Returns const char pointer */
extern char *addressToString(struct sockaddr *addr);    /* Returns static char array  */
extern char *inetAddressToString(struct sockaddr_in *addr); /* Returns ptr to sys mem */
extern void printError(char *errorMessage, int errorNumber);
extern void printInterface(struct ifaddrs *iface);
extern void dumpInterfaces();

/**
 * Must be provided by the file defining the main function.
 */
extern char *programName;

#endif
