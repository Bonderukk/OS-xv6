#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


__attribute__((noreturn))
void sieve(int left_pipe[2]) {

    int right_pipe[2];
    int prime, n;

    if (read(left_pipe[0], &prime, sizeof(int)) == 0) {
      exit(0);
    }

    printf("prime %d\n", prime);
    pipe(right_pipe);

    if(fork() == 0) {
      
      close(left_pipe[0]);
      close(right_pipe[1]);
      sieve(right_pipe);
    }
    else {

      close(right_pipe[0]);
      while(read(left_pipe[0], &n, sizeof(int)) > 0) {
        if(n % prime != 0) {
          write(right_pipe[1], &n, sizeof(int));
        }
      }
      close(right_pipe[1]);
      close(left_pipe[0]);
      wait(0);
      exit(0);
    }
}

int main(int argc, char *argv[]) {

    int first_pipe[2];
    pipe(first_pipe);

    if(fork() == 0) {

      //close(first_pipe[0]);
      close(first_pipe[1]);
      sieve(first_pipe);

    }
    else {

      close(first_pipe[0]);

      for(int i = 2; i <= 280; i++) {
        write(first_pipe[1], &i, sizeof(int));

      }
      close(first_pipe[0]);
      close(first_pipe[1]);
      wait(0);

    }

  exit(0);
}
