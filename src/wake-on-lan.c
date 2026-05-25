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

char *reduceToFirstColumn(/* in out */ char *s);
char *chomp(/*in out*/ char *s);
bool isEthAddrStr(const char *hexEthAddr);
void initWakeOnLan(/*out*/ struct WakeOnLan *w, const struct ether_addr *target);
bool initBroadcastAddress(/*out*/ struct sockaddr_in *bcAddr, const char *bcAddrStr, int port);
bool sendWakeOnLan(const struct ether_addr *target, const char *tgtHexAddr, const char *hn);
void wakeAllInFile(const char *fileName);
void handleError(const char *msg, const char *entity);
void handleError2(const char *msg, const char *entity, const char *entity2);
void handleAppError(const char *msg, const char *entity);
void handleAppError2(const char *msg, const char *entity, const char *entity2);
bool isCompleteLine(const char *line);
char *parseLine(const char *line, char *addr, size_t addrSz, char *hn, size_t hnSz);


char *pgm;
int stat;

int main(int argc, char *argv[])
{
  int i;
  struct ether_addr *sleeper;

  pgm = argv[0];
  stat = EXIT_SUCCESS;
  
  if (argc > 1)
    for (i = 1; i < argc; i++)
      if ((sleeper = ether_aton(argv[i])) != NULL)
        sendWakeOnLan(sleeper, argv[i], "<UNKNOWN>");
      else
      	wakeAllInFile(argv[i]);
  else
    wakeAllInFile("/etc/ethers");
  return stat;
}

/**
 * If the buffer for gets is too short to read the full line, only a partial
 * line will be read. Then the next read will continue reading the same line.
 * But it will read half a line. This will completely ruin input that should
 * have one entry per line. So it is necessary to be able to detect whether
 * a complete line has been read or not.
 */
bool isCompleteLine(const char *line)
{
  char lastChar;
  lastChar = line[strlen(line)-1];
  return lastChar == '\n' || lastChar == '\r';
}

char *chompBsd(/* in out */ char *s)
{
  /* Find first occurance of either \r or \n and put a terminator in it. */
  s[strcspn(s,"\r\n")] = '\0';
  return s;
}

char *chompManyCompares(char *s)
{
  char *p;
  p = s;
  while (*p && *p != '\n' && *p != '\r')
    p++;
  *p = '\0'; 
  return s;
}

char *chomp(char *s)
{
  size_t i;
  
  i = strlen(s);
  while (i > 0 && (s[--i] == '\n' || s[i] == '\r'))
    s[i] = '\0';
  return s;
}

char *reduceToFirstColumn(/* in out */ char *s)
{
  s[strcspn(s," \t\r\n")] = '\0'; /* Terminate s after the first token */
  return s;
}

char *parseLine(const char *line, char *addr, size_t addrSz, char *hn, size_t hnSz)
{
  size_t l = 0;
  size_t a = 0;
  size_t h = 0;
  size_t addrMaxLen;
  size_t hnMaxLen;
  
  addrMaxLen = addrSz - 1;
  hnMaxLen = hnSz - 1;

  while (line[l] && isspace(line[l]))                     /* Skip whitespace */
    l++;
  while (line[l] && a < addrMaxLen && !isspace(line[l]))  /* Copy to addr */
    addr[a++] = line[l++];
  while (line[l] && a == addrMaxLen && !isspace(line[l])) /* Skip rest of addr */
    l++;
  while (line[l] && isspace(line[l]))                     /* Skip whitespace */
    l++;
  while (line[l] && h < addrMaxLen && !isspace(line[l]))  /* Copy to hn */
    hn[h++] = line[l++];

  addr[a] = '\0';                                          /* Terminate */
  hn[h] = '\0';
  return addr;
}

void wakeAllInFile(const char *fileName)
{
  FILE *f;
  char line[ETH_ADDR_STR_LEN+1+FQDN_MAX_LEN+3+64]; /* Size includes line and string terminators, plus 64 extra. */
  char eth[ETH_ADDR_STR_SIZ];
  char hn[FQDN_MAX_LEN+1];
  struct ether_addr *sleeper;
  
  if ((f = fopen(fileName, "r")) != NULL) {
    while (fgets(line, sizeof(line), f) != NULL)
      if (isCompleteLine(line))
        if ((sleeper = ether_aton(parseLine(chomp(line),eth,sizeof(eth),hn,sizeof(hn)))) != NULL)
          sendWakeOnLan(sleeper, eth, hn);
        else
          handleAppError2("not a valid ethernet address", fileName, eth);
      else
        handleAppError2("ridiculously long line skipped", fileName, line);
    if (ferror(f)) /* Check required as fgets can stop on EOF or error */
      handleError("failed to read from file", fileName);
    if (fclose(f) == EOF) /* EOF means failure here */
      handleError("failed to close file", fileName);
  } else
    handleError("failed to open file", fileName);
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
  const int ETH_ADDR_LEN = 6;       /* 48-bit eth addr = 6 bytes */
  const int ETH_ADDR_REQ_REPS = 16; /* It must be repeated 16 times */
  
  int tgtIdx=0; /* Must be initialized to zero for loop correctness */
  int srcIdx;   /* Initialized by loop */
  int rep;      /* Initialized by loop */
  
  memset(w->sixMaxValBytes, 0xff, 6); /* 6 repetitions of 255 */

  for (rep = 0; rep < ETH_ADDR_REQ_REPS; rep++)
    for (srcIdx = 0; srcIdx < ETH_ADDR_LEN; srcIdx++)
      w->tgtEthAddrX16[tgtIdx++] = target->ether_addr_octet[srcIdx];
}

bool initBroadcastAddress(struct sockaddr_in *addr, const char *withAddr, int withPort)
{
  int convResult;
  bool whetherGoalAchieved = false;
  
  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  convResult = inet_pton(AF_INET, withAddr, &(addr->sin_addr));
  switch (convResult) {
  case -1:
    /* System error */
    handleError("failed to convert broadcast address", withAddr);
    break;
  case 0:
    /* The address to convert is invalid */
    handleAppError("invalid broadcast address", withAddr);
    break;
  case 1:
    /* Successful conversion */
    addr->sin_port = htons(withPort);
    whetherGoalAchieved = true;
  }
  return whetherGoalAchieved;
}

bool sendWakeOnLan(const struct ether_addr *target, const char *tgtHexStr, const char *hn)
{
  const char *BROADCAST_ADDR = "192.168.1.255";
  const int DISCARD_PORT = 9; /* WOL port doesn't matter but discard port is recommended */
  const int CLOSE_ERR = -1;
  const int ON = 1;           /* As in, to turn the broadcast socket option on. */
  
  bool whetherGoalAchieved = false;
  struct WakeOnLan *wol;
  int sock;                  /* The broadcast socket */
  struct sockaddr_in addr;   /* The broadcast address */
  
  printf("Sending WOL for %s (%s)\n", tgtHexStr, hn);
  initWakeOnLan(wol, target);
  if (initBroadcastAddress(&addr, BROADCAST_ADDR, DISCARD_PORT))  
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
      if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &ON, sizeof(ON)) != SOCK_ERR)
        if (sendto(sock, wol, sizeof(wol), 0, (sa*) &addr, sizeof(addr)) != SOCK_ERR)
          whetherGoalAchieved = true;
        else
          handleError2("failed to send wake-on-lan broadcast for", tgtHexStr, hn);
      else
        handleError2("failed to set socket to broadcast for", tgtHexStr, hn);
      if (close(sock) == CLOSE_ERR)
        handleError2("failed to close wake-on-lan socket for", tgtHexStr, hn);
    }
    else
      handleError2("failed to create wake-on-lan socket for", tgtHexStr, hn);
  return whetherGoalAchieved;
}

void handleError(const char *msg, const char *entity)
{
  fprintf(stderr, "%s: %s: %s: %s\n", pgm, msg, entity, strerror(errno));
  stat = EXIT_FAILURE; /* Record global failure */
}

void handleError2(const char *msg, const char *entity, const char *entity2)
{
  fprintf(stderr, "%s: %s: %s: %s: %s\n", pgm, msg, entity, entity2, strerror(errno));
  stat = EXIT_FAILURE; /* Record global failure */
}

void handleAppError(const char *msg, const char *entity)
{
  fprintf(stderr, "%s: %s: %s\n", pgm, msg, entity);
  stat = EXIT_FAILURE; /* Record global failure */
}

void handleAppError2(const char *msg, const char *entity1, const char *entity2)
{
  fprintf(stderr, "%s: %s: %s: %s\n", pgm, msg, entity1, entity2);
  stat = EXIT_FAILURE; /* Record global failure */
}
