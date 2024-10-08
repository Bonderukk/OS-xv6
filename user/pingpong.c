#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int to_parent[2];
    int to_child[2];
    pipe(to_parent);
    pipe(to_child);

    
    int pid = fork();
    char *received = "b";

    if(pid == 0) {
        
        close(to_child[1]);
        close(to_parent[0]);

        read(to_child[0], &received, 1);
        printf("%d Received ping\n", getpid());
        write(to_parent[1], &received, 1);

        close(to_child[0]);
        close(to_parent[1]);
    }
    else {

        close(to_child[0]);
        close(to_parent[1]);

        write(to_child[1], &received, 1);
        read(to_parent[0], &received, 1);
        printf("%d Received pong\n", getpid());

        close(to_child[1]);
        close(to_parent[0]);
    }

    exit(0);

    
    
}