#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

void check_signal(int sig){
    if(sig==SIGINT){
        printf("Am primit SIGINT -> Inchidere program\n");
        if(unlink(".monitor_pid")==-1){
            perror(NULL);
        }
        exit(0);
    }
    else if(sig==SIGUSR1){
        printf("Am primit SIGUSR1 -> A fost adaugat un raport\n");
    }
}

int main(){
    int f=open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(f==-1){
        perror(NULL);
        exit(-1);
    }
    pid_t pid=getpid();
    char pid_str[16];
    int len=snprintf(pid_str, sizeof(pid_str), "%d", pid);
    if (write(f, pid_str, len)==-1){
        perror(NULL);
        close(f);
        exit(-1);
    }
    close(f);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler=check_signal;
    sa.sa_flags=SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    while(1){
        pause();
    }
    return 0;
}
