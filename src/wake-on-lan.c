/* wake-on-lan.c */
#include <stdio.h>   /* printf, fprintf */
#include <stdbool.h> /* bool, true, false */
#include <stdlib.h>  /* atoi, exit, EXIT_FAILURE */
#include <string.h>  /* memset */
#include <unistd.h>  /* close */
#include <libgen.h>  /* basename */
#include <errno.h>   /* errno */
#include <limits.h>  /* POSIX HOST_NAME_MAX */

#include "cross-platform-sockets.h"

struct WakeOnLan
{
  uint8_t sixMaxValBytes[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  /* 16 repetitions of the 48-bit target mac address:
     16 * 48 = 768 bits = 96 bytes = 24 32-bit words */
  uint8_t tgtEthAddrX16[96];
};

char *chomp(/*out*/ char *s);
bool isEthAddrStr(const char *hexEthAddr);
void initWakeOnLan(/*out*/ struct WakeOnLan *w, const struct ether_addr *target);
void initBroadcastAddress(/*out*/ struct sockaddr_in *bcAddr);
bool sendWakeOnLan(const struct ether_addr *target, const char *tgtHexAddr);
bool wakeAllInFile(const char *fName);

char *pgm;

int main(int argc, char *argv[])
{
  int exitCode = EXIT_SUCCESS;
  int i;
  FILE *f;
  line[ETH_ADDR_LINE_SIZ]; /* Includes line and string terminators. */
  struct ether_addr *sleeper;

  pgm = argv[0];
  
  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      /* Check for ethernet mac address on cmd line before file name. */
      sleeper = ether_aton(argv[i]);
      if (sleeper != NULL)
        exitCode = sendWakeOnLan(sleeper, argv[i]) ? EXIT_SUCCESS : EXIT_FAILURE;
      else {
        f = fopen(argv[i], "r");
        if (f != NULL) {
          while (fgets(line, sizeof(line), f) != NULL) {
            sleeper = ether_aton(chomp(line));
            if (sleeper != NULL)
              exitCode = sendWakeOnLan(sleeper, line) ? EXIT_SUCCESS : EXIT_FAILURE;
            else
              fprintf(stderr, "%s: not a valid ethernet address: %s\n", pgm, line);
          }
          /* Check if fgets caused an error rather than reaching the end-of-file. */
          if (ferror(f))
            fprintf(stderr, "%s: fgets failed for file: %s: %s\n", pgm, argv[i], strerror(errno));
          fclose(f);
        } else
          fprintf(stderr, "%s: %s: %s\n", pgm, argv[i], strerror(errno));
      }
    }
  } else {
    fprintf(stderr, "Wake up what? Dumbass...\n");
    exitCode = EXIT_FAILURE;
  }
  return exitCode;
}

char *chomp(char *s)
{
  /* Find first occurance of either \r or \n and put a terminator in it. */
  s[strcspn(s,"\r\n")] = '\0';
  return s;
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

  for (rep = 0; rep < WOL_ETH_ADDR_REQ_REPETITIONS; rep++)
    for (srcIdx = 0; srcIdx < ETH_ADDR_BYTE_LEN; srcIdx++)
      w.tgtEthAddrX16[tgtIdx++] = target.ether_addr_octet[srcIdx];
}

void initBroadcastAddress(struct sockaddr_in *bcAddr)
{
  memset(bcAddr, 0, sizeof(struct sockaddr_in));
  bcAddr->sin_family = AF_INET;
  /* TODO - Can this fail? */
  inet_pton(AF_INET, "255.255.255.255", &(bcAddr->sin_addr));
  bcAddr->sin_port = htons(SERVER_PORT);
}

bool sendWakeOnLan(const struct ether_addr *target, const char *tgtHexStr)
{
  const int BROADCAST_ON = 1;
  
  bool isSuccessful = false;
  struct WakeOnLan *tgtWol;
  int bcSock;                 /* The broadcast socket */
  struct sockaddr_in bcAddr;  /* The broadcast address */
  
  initWakeOnLan(tgtWol, target);
  initBroadcastAddress(&bcAddr);
  
  if ((bcSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
    if (setsockopt(bcSock, SOL_SOCKET, SO_BROADCAST, &BROADCAST_ON, sizeof(BROADCAST_ON)) != SOCK_ERR)
      if (sendto(bcSock, tgtWol, sizeof(tgtWol), 0, (sa*) &bcAddr, sizeof(bcAddr)) != SOCK_ERR)
        isSuccessful = true;
      else
        fprintf(stderr, "%s: failed to broadcast wake-on-lan for: %s: %s\n", pgm, tgtHexStr, strerror(errno));
    else
      fprintf(stderr, "%s: failed to set socket to broadcast: %s\n", pgm, strerror(errno));
    /* Clean-up file descriptor, only if it was successfully opened. */
    fsync(bcSock);
    close(bcSock);
  } else
    fprintf(stderr, "%s: Failed to create socket for wake-on-lan broadcast: %s\n", pgm, strerror(errno));
  return isSuccessful;
}

/* Broadcast Eth addr so that other machines can learn of its existance.
   Since other machines may be sleeping, you can't discover their eth
   addr. They need to provide it when they are awake so that it can be
   stored to be used later. */
void sendEthAddrBroadcast()
{
}
