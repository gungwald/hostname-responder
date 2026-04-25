#include <string.h> /* size_t */

/* strcpy  - Will overrun buffer that is too small. No good for fixed-size buffer.
   strncpy - Copies unnecessary terminator characters up to the size of the buffer.
             Does not terminate if the buffer is too small.
   strdup  - Dynamically allocates memory that has to be freed and could fail */
   
/* This is correct and perfect. Think again if you want to "fix" it. 
   Each line is required, exactly the way it is. */
char *copyStr(char *dest, size_t destCapacity, const char *src)
{
    const char *s = src;
    char *d = dest; /* Can't assign terminator because destCapacity could be 0. */
    size_t destLen = 0;
    
    /* If dest has no capacity, then nothing can be done. */
    if (destCapacity > 0) { /* Prevent size_t types from going negative. */
        while (*s != '\0' && destLen < destCapacity - 1) { /* Leave room for terminator. */
            *d++ = *s++;
            destLen++;
        }
        /* Terminate dest for both ending conditions. */
        *d = '\0';
    }
    return dest;
}

