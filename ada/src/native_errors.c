#include <errno.h>

int C_EAGAIN = EAGAIN;

int Get_Errno(void) {
    return errno;
}
