#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

void handle_hub_mon(int pf[2]){
    close(pf[1]);
    FILE *stream=fdopen(pf[0], "r");
    if (!stream){
        perror(NULL);
        exit(1);
    }
    char buf[512];
    while(fgets(buf, sizeof(buf), stream)!=NULL){
        char test=buf[0];
        char *msg= buf+1;
        if(test=='E'){
            printf("\n[HUB Error] %s", msg);
            fflush(stdout);
        }
        else if(test=='V'){
            printf("\n[HUB Info] %s", msg);
            fflush(stdout);
        }
        else{
            printf("\n[HUB] %s", buf);
            fflush(stdout);
        }
    }
    printf("\n[HUB Note] Procesul monitor_reports s-a terminat\n");
    fflush(stdout);
    fclose(stream);
    exit(0);
}

void monitor_command(){
    pid_t pid_hub_mon=fork();
    if(pid_hub_mon==-1){
        perror(NULL);
        return;
    }
    if(pid_hub_mon==0){
        int pf[2];
        if(pipe(pf)==-1){
            perror(NULL);
            exit(1);
        }
        pid_t pid_monitor=fork();
        if (pid_monitor == -1) {
            perror(NULL);
            exit(1);
        }
        if(pid_monitor==0){
            close(pf[0]);
            if(dup2(pf[1], STDOUT_FILENO)==-1){
                perror(NULL);
                exit(1);
            }
            close(pf[1]);
            execlp("./monitor_reports", "./monitor_reports", NULL);
            perror(NULL);
            exit(1);
        }
        else{
            handle_hub_mon(pf);
        }
    }
    else{
        printf("Comanda start_monitor a fost lansata in background (hub_mon PID: %d)\n", pid_hub_mon);
    }
}

void calculate_scores(char *command_args){
    char *districts[32];
    int count=0;
    char *p=strtok(command_args, " ");
    while(p!=NULL && count<32){
        districts[count++]=p;
        p=strtok(NULL, " ");
    }
    if(count==0){
        printf("Niciun district mentionat\n");
        return;
    }
    int pipes[32][2];
    pid_t pids[32];
    for(int i=0;i<count;i++){
        if(pipe(pipes[i])==-1){
            perror(NULL);
            return;
        }
        pids[i]=fork();
        if(pids[i]==-1){
            perror(NULL);
            return;
        }
        if(pids[i]==0){
            close(pipes[i][0]);
            if(dup2(pipes[i][1], STDOUT_FILENO)==-1){
                perror(NULL);
                exit(1);
            }
            close(pipes[i][1]);
            for(int j=0;j<i;j++){
                close(pipes[j][0]);
            }
            execlp("./scorer", "./scorer", districts[i], NULL);
            perror(NULL);
            exit(1);
        }
        else{
            close(pipes[i][1]);
        }
    }
    char buf[256];
    for(int i=0;i<count;i++){
        FILE *stream=fdopen(pipes[i][0], "r");
        if(stream){
            while(fgets(buf, sizeof(buf), stream)!=NULL){
                char dist[64], insp[64];
                int score;
                if(sscanf(buf,"%[^:]:%[^:]:%d", dist, insp, &score)==3){
                    printf("%s   %s   %d\n", dist, insp, score);
                }
            }
            fclose(stream);
        }
        waitpid(pids[i], NULL, 0);
    }
}

int main(){
    char command[128];
    while(1){
        printf("city_hub> ");
        fflush(stdout);
        if(fgets(command, sizeof(command), stdin)==NULL){
            break;
        }
        command[strcspn(command, "\n")]=0;
        if(strcmp(command, "exit")==0){
            printf("Inchidere city_hub\n");
            break;
        }
        else if(strcmp(command, "start_monitor")==0){
            monitor_command();
        }
        else if(strncmp(command, "calculate_scores", 16)==0){
            char *args=command+16;
            while(*args==' ')
                args++;
            calculate_scores(args);
        }
        else if(strlen(command)>0){
            printf("Comanda necunoscuta\n");
        }
    }
    return 0;
}
