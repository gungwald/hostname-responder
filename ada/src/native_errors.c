#include <errno.h>

int C_EAGAIN = EAGAIN;
int C_EBADF  = EBADF;

int Get_Errno(void) {
    return errno;
}
