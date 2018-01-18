#include <unistd.h> /* gethostname */

/* This is not defined on MacOS X 10.4.11 even though it is mentioned in
   the man page for gethostname. */
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 511
#endif

char *getHostName();
char *programName;

int main(int argc, char *argv[])
{
    programName = argv[0];
    /* Listen to port and send back hostname. */
    puts(getHostName());
}

char *getHostName()
{
    const int GETHOSTNAME_ERR = -1;
    static char hostname[HOST_NAME_MAX+1];

    if (gethostname(hostname, HOST_NAME_MAX) == GETHOSTNAME_ERR) {
        perror(programName);
    }
    /* NULL termination is not guaranteed by gethostname if the destination
       is not big enough, so make sure the string is terminated if data is
       equal to or greater than HOST_NAME_MAX. */
    hostname[HOST_NAME_MAX] = '\0';
    return hostname;
}

