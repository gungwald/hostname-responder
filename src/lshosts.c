/* lshosts */

#include <stdio.h>      /* printf */
#include <stdlib.h>     /* EXIT_SUCCESS,EXIT_FAILURE */
#include <string.h>     /* memset */
#include <errno.h>      /* perror */
#include <sys/types.h>  /* Needed for BSD but not Linux */
#include <sys/socket.h> /* socket,setsockopt */
#include <arpa/inet.h>  /* struct sockaddr_in */

struct sockaddr *buildSubnetBroadcastAddress(struct sockaddr_in *address);

const int SOCK_ERR = -1;
const int REQUEST_PORT = 4140;

int main(int argc, char *argv[])
{
    int commChannel;
    int on = 1;
    struct sockaddr_in subnetInetAddr;
    struct sockaddr *subnetAddr;
    int exitStatus = EXIT_FAILURE;
    char *data = "What is your name?";

    /* Open a communications channel. */
    if ((commChannel = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP)) != SOCK_ERR){
        /* Configure it to broadcast to multiple receivers. */
        if (setsockopt(commChannel,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on)) != SOCK_ERR) {
            /* Build the address for the intended receivers, which are all hosts on this subnet.*/
            subnetAddr = buildSubnetBroadcastAddress(&subnetInetAddr);
            /* Broadcast the data to the whole subnet.*/
            if (sendto(commChannel,data,strlen(data),0,subnetAddr,sizeof(subnetInetAddr)) != SOCK_ERR) {
                exitStatus = EXIT_SUCCESS;
            }
        }
        close(commChannel);
    } 

    if (exitStatus == EXIT_FAILURE) {
        perror(argv[0]);
    }
    return exitStatus;
}

struct sockaddr *buildSubnetBroadcastAddress(struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = inet_addr();
    address->sin_port = htons(REQUEST_PORT);
}

