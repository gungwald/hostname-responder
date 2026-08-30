/* hwætd.c */

/* System headers */
#include <stdio.h>    /* printf, fprintf */
#include <stdbool.h>  /* bool, true, false */
#include <stdlib.h>   /* atoi, exit, EXIT_FAILURE */
#include <string.h>   /* memset, strdup */
#include <signal.h>   /* signal, SIGCHLD, SIGHUP, SIG_IGN  */
#include <unistd.h>   /* close, POSIX gethostname */
#include <libgen.h>   /* basename */
#include <errno.h>    /* errno */
#include <syslog.h>   /* openlog, syslog, closelog */
#include <sys/stat.h> /* umask */
#include <limits.h>   /* NAME_MAX */

#include "cross-platform-sockets.h"
#include "hwaet-common.h"
#include "string-ops.h"


void becomeDaemon();
char *getHostnameNoFailure();
bool getIpAddr(char *ipAddr, size_t capacity);
bool getHostIdentification(char *ident, size_t capacity);
bool runSocketServer(const char *hostname);
bool processRequests(const char *hostname, int sSock, int cSock);
void initReceiptAddress(struct sockaddr_in *address, in_port_t port);
bool isPrimaryInterface(struct ifaddrs *i);
bool isLoopback(struct sockaddr *address);
bool findPrimaryInterface(struct ifaddrs *i);
void signalHandler(int signal);
void closeSocket(int sock, const char *desc);

/**
 * Avoids putting a mysterious zero in the middle of the socket function calls.
 */
const int NO_FLAGS=0;

/* NAME_MAX is in bytes, not chars. This makes a difference with multibyte
   encodings like UTF-8. NAME_MAX does not include the terminator byte.
   So one byte needs to be added for that. */
char programName[NAME_MAX+1];
bool fatalErrorOccurred = false;
int sSock = -1;
int cSock = -1;

/* This needs to be removed. */
bool noErrors = true;

int main(int argc, char *argv[])
{
  char hostIdent[HOST_NAME_MAX+INET_ADDRSTRLEN+2]; /* Space and terminator */
  bool success = false;
  int exitCode;

  openlog(programName, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);

  copyStr(programName, sizeof(programName), basename(argv[0]));

  if (strcmp(argv[1],"--background")==0)
    becomeDaemon();

  if (runSocketServer(getHostnameNoFailure()))
    success = true;
  
  exitCode = success ? EXIT_SUCCESS : EXIT_FAILURE;
  syslog(LOG_INFO, "Returning from main with exit code: %d", exitCode);
  closelog();
  return exitCode;
}

void closeSocket(int sock, const char *desc) {
  /* A negative number would indicate that the socket is not open. */
  if (sock >= 0) {
    if (close(sock) == -1)
      syslog(LOG_ERR, "Failed to close client socket: %m");
  } else {
    syslog(LOG_WARNING, "Skipping close of socket because it is not open");
  }
}

void signalHandler(int signal)
{
  int errnoBeforeSignal;
  errnoBeforeSignal = errno;

  syslog(LOG_INFO, "Caught signal %d. Exiting.", signal);
  closeSocket(cSock, "Client socket");
  closeSocket(sSock, "Server socket");
  closelog();
  exit(0);

  errno = errnoBeforeSignal;
}

void becomeDaemon()
{
  pid_t pid;
  int fd;

  /* Fork off the parent process */
  pid = fork();

  /* Fork failed */
  if (pid < 0) {
    syslog(LOG_ERR, "Failed to fork: %m");
    closelog()
    exit(EXIT_FAILURE);
  }

  /* Success: Let the parent terminate */
  if (pid > 0) {
    // This is the parent process. It can safely exit now.
    syslog(LOG_INFO, "Parent successfully spawned child (PID: %d). Exiting.", pid);
        
    // DO NOT call closelog() here. Just exit.
    exit(EXIT_SUCCESS);
  }

  /* On success: The child process becomes session leader */
  if (setsid() < 0) {
    fprintf(stderr, "%s: Failed to create new session: %s", programName, strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Catch, ignore and handle signals */
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGTERM, signalHandler);

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if (pid < 0)
    exit(EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0)
    exit(EXIT_SUCCESS);

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory
     or another appropriated directory */
  chdir("/tmp");

  /* Close all open file descriptors */
  for (fd = sysconf(_SC_OPEN_MAX); fd>=0; fd--)
    close(fd);
}

bool getHostIdentification(char *ident, size_t capacity)
{
  char *hostname;
  char ipAddr[INET_ADDRSTRLEN]; /* Long enough for terminator on OpenBSD */

  if ((hostname = getHostnameNoFailure()) != NULL) {
    if (getIpAddr(ipAddr, sizeof(ipAddr)))
      snprintf(ident, capacity, "%s %s", hostname, ipAddr);
  }
  else
    copyStr(ident, sizeof(ident), "UNKNOWN");

  return !fatalErrorOccurred;
}

bool getIpAddr(char *ipAddr, size_t capacity)
{
  struct ifaddrs ifPrime;

  if (findPrimaryInterface(&ifPrime))
    if (inet_ntop(AF_INET, &(((sain*)ifPrime.ifa_addr)->sin_addr), ipAddr, capacity) == NULL) {
      syslog(LOG_ERR, "Failed to get IP address of primary interface: %s: %m", ifPrime.ifa_name);
      fatalErrorOccurred = true;
    }

  return !fatalErrorOccurred;
}

/**
 * Returns an internal statically allocated string. It will never return
 * NULL. It returns an error string on failure. This is a gethostname
 * wrapper to avoid the need for error handling. If you want to determine
 * success or failure, call gethostname directly.
 */
char *getHostnameNoFailure()
{
  enum GetHostnameResult { GHN_FAILURE=-1, GHN_SUCCESS=0 };
  const char *onFailure = "(system failed to get hostname)";

  /* POSIX value, HOST_NAME_MAX, does not include the string terminator
     character, so 1 is added to length to allow room for the terminator. */
  static char hostname[HOST_NAME_MAX+1] = {'\0'}; /* Prevent garbage */
  size_t hnSz;

  hnSz = sizeof(hostname);

  if (gethostname(hostname, hnSz) == GHN_SUCCESS) {
    /* OpenBSD guarantees that the gethostname string parameter will end
       with a terminator character, but not all operating systems do. So,
       the string terminator character will be added to make sure it
       always works on every system. */
    hostname[hnSz-1] = '\0';
  } else {
    syslog(LOG_ERR, "Failed to get hostname: %m");
    copyStr(hostname, hnSz, onFailure);
  }
  return hostname;
}


bool runSocketServer(const char *hostname)
{
  struct sockaddr_in svrAddr;
  bool success = false;

  if ((sSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
    syslog(LOG_INFO, "Created server socket with file descriptor: %d", sSock);
    initReceiptAddress(&svrAddr, SERVER_PORT);
    if (bind(sSock, (sa*) &svrAddr, sizeof(svrAddr)) != SOCK_ERR) {
      syslog(LOG_INFO, "Bound socket to port: %s", ipAddr2Str(&svrAddr));
      if ((cSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        syslog(LOG_INFO, "Created client socket with file descriptor: %d", cSock);
        if (processRequests(hostname, sSock, cSock))
          success = true;
        close(cSock);
      } else
        syslog(LOG_ERR, "Failed to open client socket: %m");
    } else
      syslog(LOG_ERR, "Failed to bind to port: %s: %m", ipAddr2Str(&svrAddr));
    close(sSock);
  } else
    syslog(LOG_ERR, "Failed to create server socket: %m");
  return success;
}

/* msg must be terminated. */
void sendMessage(int sock, const char *msg, struct sockaddr_in *addr)
{
  size_t msgSz;
  socklen_t addrSz;

  msgSz = strlen(msg) + 1;
  addrSz = (socklen_t) sizeof(*addr);

  if (sendto(sock, msg, msgSz, NO_FLAGS, (sa*) addr, addrSz) != SOCK_ERR)
    syslog(LOG_INFO, "Sent message '%s' to %s", msg, ipAddr2Str(addr));
  else
    syslog(LOG_ERR, "Failed to send message '%s' to %s: %m", msg, ipAddr2Str(addr));
}

/**
 * Wait for and respond to all requests forever unless an error occurs.
 *
 * @param hn The hostname of this computer
 * @param sSock The server (listening) socket on this machine
 * @param cSock The client socket on remote machine
 */
bool processRequests(const char *hn, int sSock, int cSock)
{
  /* cAddr is large enough to handle any type of address. This is important
     because the recvfrom function's "from" parameter is an input/output
     parameter, so the client can send back any type of address, which could
     be many different sizes. */
  struct sockaddr_storage cAddr;
  struct sockaddr_in *cAddrIn;
  socklen_t cAddrSz;
  char msg[32];
  size_t msgSz;
  size_t hnSz;
  bool success = true;
  const size_t MAX_RESP = 1024;
  char resp[MAX_RESP];

  cAddrSz = sizeof(cAddr);
  msgSz = sizeof(msg);
  hnSz = strlen(hn) + 1;

  while (success) {
    syslog(LOG_INFO, "Waiting for requests");
    if (recvfrom(sSock, msg, msgSz, NO_FLAGS, (sa*) &cAddr, &cAddrSz) != SOCK_ERR) {
      msg[31] = '\0'; /* Make sure it is terminated so it can be logged. */
      /* Check that cAddr is the right type. */
      if (cAddr.ss_family == AF_INET) {
        /* Cast once for convenience */
        cAddrIn = (struct sockaddr_in *) &cAddr;
      	syslog(LOG_INFO, "Received message from: %s: %s", ipAddr2Str(cAddrIn), msg);
        cAddrIn->sin_port = htons(CLIENT_PORT);
        syslog(LOG_INFO, "Changed port in address: %s", ipAddr2Str(cAddrIn));
        if (strncmp(msg,"Hw\xE6t",4)==0) {
          sendMessage(cSock, hn, cAddrIn);
        } else if (strncmp(msg,"Hwaet",5)==0) {
          snprintf(resp, sizeof(resp), "%s (from inferior request)", hn);
          sendMessage(cSock, resp, cAddrIn);
        } else if (strlen(msg)==0) {
          sendMessage(cSock, "An empty message is an invalid request.", cAddrIn);
        } else {
          snprintf(resp, sizeof(resp), "The message '%s' is an invalid request. You must speak Old English.", msg);
          sendMessage(cSock, resp, cAddrIn);
        }
      } else {
        /* This code cannot yet process connections from a non-IPv4
           client because it can't create an address and port
           for other types of addresses. */
        syslog(LOG_INFO, "Client is using a non-IPv4 address family: %d", cAddr.ss_family);
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
    if (!found)
      syslog(LOG_ERR, "Search for primary interface failed to find one");
  } else
    syslog(LOG_ERR, "Failed to get network interfaces: %m");
  fatalErrorOccurred = !found;
  return found;
}
