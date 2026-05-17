/* wake-on-lan */
#include <stdio.h>      /* printf, fprintf */
#include <stdbool.h>	/* bool, true, false */
#include <stdlib.h>     /* atoi, exit, EXIT_FAILURE */
#include <string.h>     /* memset */
#include <unistd.h>     /* close */
#include <libgen.h>	/* basename */
#include <errno.h>	/* errno */
#include <limits.h>	/* POSIX HOST_NAME_MAX */

#include "cross-platform-sockets.h"

char *chomp(char *s);
bool isEthAddrStr(cons char *hexEthAddr);
void sendWakeOnLan(const char *hexEthAddr);

int main(int argc, char *argv[])
{
    int exitCode = EXIT_SUCCESS;
    int i;
    FILE *f;
    line[ETHER_ADDR_STR_SIZ+2]; /* Includes string term char, +2 for line endings. */
    
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (isEthAddrStr(argv[i])) {
                sendWakeOnLan(argv[i]);
            } else {
                f = fopen(argv[i], "r");
                if (f != NULL) {
                    while (fgets(line, sizeof(line), f) != NULL) {
                        sendWakeOnLan(chomp(line));
                    }
                    if (ferror(f)) {
                        fprintf(stderr, "%s: fgets failed for file: %s: %s\n", argv[0], argv[i], strerror(errno));
                    }
                    fclose(f);
                } else {
                    perror(argv[i]);
                }
            }
        }
    } else {
        fprintf(stderr, "Wake what, dumbass?\n");
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

bool isEthAddrStr(cons char *s)
{
  int i;

  for (i = 0; i < ETH_ADDR_STR_LEN; i++)
    if ((i + 1) % 3 == 0)
      if (s[i] == ':')
        continue;
      else
        return false;
    else
      if (isxdigit(s[i]))
        continue;
      else
        return false;
  return true;
}

void sendWakeOnLan(const char *hexEthAddr)
{
    if ((bcastSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
        if (sendHostnameBrodcastRequest(bcastSock)) {
            if ((responseSock = socket(AF_INET, SOCK_DGRAM, 0)) != SOCK_ERR) {
                readResponses(responseSock);
                close(responseSock);
            } else {
                handleError("Failed to create socket for receiving hostname responses", NULL, errno);
            }
        }
        close(bcastSock);
    } else {
        handleError("Failed to create socket for broadcasting hostname request", NULL, errno);
    }

}

/* Broadcast Eth addr so that other machines can learn of its existance. 
   Since other machines may be sleeping, you can't discover their eth
   addr. They need to provide it when they are awake so that it can be
   stored to be used later. */
void sendEthAddrBroadcast()
{
}
