#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <signal.h>

#define MAXARGS 20
#define ARGLEN 100

int execute(char * arglist[]);
char *mkstring(char *buf);

int main(){
    char *arglist[MAXARGS + 1];
    int num = 0;
    char argbuf[ARGLEN];
    char *mkstring();

    while(num < MAXARGS){
        printf("Arg[%d]?",num);
        if(fgets(argbuf,ARGLEN,stdin) && *argbuf != '\n')
            arglist[num++] = mkstring(argbuf);
        else{
            if(num > 0){
                arglist[num] = NULL;
                execute(arglist);
                num = 0;
            }
        }
    }
    return 0;
}
int execute(char * arglist[]){
    execvp(arglist[0],arglist);
    perror("execvp failed");
    exit(1);
}
char *mkstring(char *buf){
    char *cp;
    buf[strlen(buf) - 1] = '\0';
    cp = malloc(strlen(buf) + 1);
    if(cp == NULL){
        fprintf(stderr,"no memory\n");
        exit(1);
    }
    strcpy(cp,buf);
    return cp;
}