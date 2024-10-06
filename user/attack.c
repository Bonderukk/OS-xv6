#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"
#include "kernel/param.h"


// Define the size of the secret
#define SECRET_SIZE 8

int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}


int
main(int argc, char *argv[])
{
    char *start = sbrk(PGSIZE * 32);
    char *end = start + PGSIZE * 32;
    const char *target = "very very secret pw is:";

    for (char *ptr = start; ptr < end - SECRET_SIZE; ptr++) {
        if (strncmp(ptr, target, strlen(target)) == 0) {
            char *after_colon = ptr + strlen(target) + 1;
            if (strlen(after_colon) == 7) {
                write(2, after_colon, 8);
                break;
            }
        }
    }
    exit(0);
}
