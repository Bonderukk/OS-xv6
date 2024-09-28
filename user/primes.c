#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


__attribute__((noreturn))
void sieve(int left_pipe[2]) {
  int prime, n;
  int right_pipe[2];

  // Ked uz nie je co citat, skonci
  if (read(left_pipe[0], &prime, sizeof(int)) == 0) {
    exit(0);
  }

  printf("prime %d\n", prime);

  // Vytvor novú rúru pre pravého suseda
  pipe(right_pipe);

  if (fork() == 0) {
    // Dieťa
    close(left_pipe[0]);
    close(right_pipe[1]);
    sieve(right_pipe);
  } else {
    // Rodič
    close(right_pipe[0]);
    
    // Čítaj čísla z ľavej rúry a posielaj ďalej tie, ktoré nie sú deliteľné prvočíslom
    while (read(left_pipe[0], &n, sizeof(int)) > 0) {
      if (n % prime != 0) {
        write(right_pipe[1], &n, sizeof(int));
      }
    }

    // Uzavri rúry a počkaj na ukončenie dieťaťa
    // Je to fajn aby sa uvolnili fd
    close(left_pipe[0]);
    close(right_pipe[1]);
    wait(0);
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  int first_pipe[2];
  pipe(first_pipe);

  if (fork() == 0) {
    // Dieťa
    close(first_pipe[1]);
    sieve(first_pipe);
  } else {
    // Rodič
    close(first_pipe[0]);
    
    // Generuj čísla od 2 do 280 a posielaj ich do rúry
    for (int i = 2; i <= 280; i++) {
      write(first_pipe[1], &i, sizeof(int));
    }

    // Uzavri zapisovaciu stranu rúry a počkaj na ukončenie dieťaťa
    close(first_pipe[1]);
    wait(0);
  }

  exit(0);
}
