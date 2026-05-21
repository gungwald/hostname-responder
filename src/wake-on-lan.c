/* wake-on-lan.c */
#include <stdio.h>   /* printf, fprintf */
#include <stdbool.h> /* bool, true, false */
#include <stdlib.h>  /* atoi, exit, EXIT_FAILURE */
#include <string.h>  /* memset */
#include <ctype.h>   /* isxdigit */
#include <unistd.h>  /* close */
#include <libgen.h>  /* basename */
#include <errno.h>   /* errno */
#include <limits.h>  /* POSIX HOST_NAME_MAX */

#include "cross-platform-sockets.h"

struct WakeOnLan
{
  uint8_t sixMaxValBytes[6];
  /* 16 repetitions of the 48-bit target mac address:
     16 * 48 = 768 bits = 96 bytes = 24 32-bit words */
  uint8_t tgtEthAddrX16[96];
};

char *chomp(/*out*/ char *s);
bool isEthAddrStr(const char *hexEthAddr);
void initWakeOnLan(/*out*/ struct WakeOnLan *w, const struct ether_addr *target);
void initBroadcastAddress(/*out*/ struct sockaddr_in *bcAddr, int port);
bool sendWakeOnLan(const struct ether_addr *target, const char *tgtHexAddr);
void wakeAllInFile(const char *fName);
void handleError(const char *msg, const char *entity);
void handleAppError(const char *msg, const char *entity, const char *entity2);


char *pgm;
int stat;

int main(int argc, char *argv[])
{
  int i;
  struct ether_addr *sleeper;

  pgm = argv[0];
  stat = EXIT_SUCCESS;
  
  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      /* Check for ethernet mac address on cmd line before file name. */
      sleeper = ether_aton(argv[i]); /* Tests if it is an ethernet address. */
      if (sleeper != NULL)
        sendWakeOnLan(sleeper, argv[i]);
      else
      	wakeAllInFile(argv[i]);
    }
  } else
    wakeAllInFile("/etc/ethers");
  return stat;
}

char *chomp(char *s)
{
  /* Find first occurance of either \r or \n and put a terminator in it. */
  s[strcspn(s,"\r\n")] = '\0';
  return s;
}

void wakeAllInFile(const char *fName) {
  FILE *f;
  char line[ETH_ADDR_LINE_SIZ]; /* Includes line and string terminators. */
  struct ether_addr *sleeper;
  
  if ((f = fopen(fName, "r")) != NULL) {
    while (fgets(line, sizeof(line), f) != NULL) {
      if ((sleeper = ether_aton(chomp(line))) != NULL)
        sendWakeOnLan(sleeper, line);
      else
        handleAppError("not a valid ethernet address", fName, line);
    }
    if (ferror(f)) /* Check if fgets caused an error, rather than reaching EOF. */
      handleError("failed to read from file", fName);
    if (fclose(f) == EOF) /* Close and check for failure. EOF means failure here. */
      handleError("failed to close file", fName);
  } else
    handleError("failed to open file", fName);
}

/**
 * Will only work if all single digits have leading zeros.
 */
bool isEthAddrStr(const char *s)
{
  int i;

  for (i = 0; i < ETH_ADDR_STR_LEN; i++)
    if ((i + 1) % 3 == 0)
      if (s[i] == ':')
        continue;
      else
        return false;
    else if (isxdigit(s[i]))
      continue;
    else
      return false;
  return true;
}

void initWakeOnLan(struct WakeOnLan *w, const struct ether_addr *target)
{
  const int ETH_ADDR_BYTE_LEN = 6; /* 48-bit eth addr = 6 bytes */
  const int WOL_ETH_ADDR_REQ_REPETITIONS = 16;
  
  int tgtIdx=0; /* Must be initialized to zero for loop correctness */
  int srcIdx;   /* Initialized by loop */
  int rep;      /* Initialized by loop */
  
  memset(w->sixMaxValBytes, 0xff, 6); /* 6 repetitions of 255 */

  for (rep = 0; rep < WOL_ETH_ADDR_REQ_REPETITIONS; rep++)
    for (srcIdx = 0; srcIdx < ETH_ADDR_BYTE_LEN; srcIdx++)
      w->tgtEthAddrX16[tgtIdx++] = target->ether_addr_octet[srcIdx];
}

void initBroadcastAddress(struct sockaddr_in *bcAddr, int port)
{
  memset(bcAddr, 0, sizeof(struct sockaddr_in));
  bcAddr->sin_family = AF_INET;
  /* TODO - Can this fail? */
  inet_pton(AF_INET, "255.255.255.255", &(bcAddr->sin_addr));
  bcAddr->sin_port = htons(port);
}

bool sendWakeOnLan(const struct ether_addr *target, const char *tgtHexStr)
{
  const int FSYNC_ERR = -1;
  const int CLOSE_ERR = -1;
  const int BROADCAST_ON = 1;
  
  bool isSuccessful = false;
  struct WakeOnLan *tgtWol;
  int bcSock;                 /* The broadcast socket */
  struct sockaddr_in bcAddr;  /* The broadcast address */
  
  initWakeOnLan(tgtWol, target);
  initBroadcastAddress(&bcAddr, 0);
  
  if ((bcSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
    if (setsockopt(bcSock, SOL_SOCKET, SO_BROADCAST, &BROADCAST_ON, sizeof(BROADCAST_ON)) != SOCK_ERR)
      if (sendto(bcSock, tgtWol, sizeof(tgtWol), 0, (sa*) &bcAddr, sizeof(bcAddr)) != SOCK_ERR)
        isSuccessful = true;
      else
        handleError("failed to send wake-on-lan broadcast", tgtHexStr);
    else
      handleError("failed to set socket to broadcast", tgtHexStr);
    if (fsync(bcSock) == FSYNC_ERR)
      handleError("failed to flush wake-on-lan socket", tgtHexStr);
    if (close(bcSock) == CLOSE_ERR)
      handleError("failed to close wake-on-lan socket", tgtHexStr);
  } else
    handleError("failed to create socket for wake-on-lan", tgtHexStr);
  return isSuccessful;
}

void handleError(const char *msg, const char *entity)
{
  fprintf(stderr, "%s: %s: %s: %s\n", pgm, msg, entity, strerror(errno));
  stat = EXIT_FAILURE;
}

void handleAppError(const char *msg, const char *entity1, const char *entity2)
{
  fprintf(stderr, "%s: %s: %s: %s\n", pgm, msg, entity1, entity2);
  stat = EXIT_FAILURE;
}
