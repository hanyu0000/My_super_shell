#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>             

#define MAX_CMD 10
#define BUFFSIZE 100
#define MAX_CMD_LEN 100

int argc;                                       // 有效参数个数
char* argv[MAX_CMD];                            // 参数数组
char command[MAX_CMD][MAX_CMD_LEN];             // 参数数组
char buf[BUFFSIZE];                             // 接受输入的参数数组
char backupBuf[BUFFSIZE];                       // 参数数组的备份
char curPath[BUFFSIZE];                         // 当前工作路径
int i, j;                                       
int commandNum;                                 // 已经输入指令数目
char history[MAX_CMD][BUFFSIZE];                // 存放历史命令
void sig_handler(int signum);
int get_input(char buf[]);                      
void parse(char* buf);                          
void do_cmd(int argc, char* argv[]);
void Cd(char command[MAX_CMD][MAX_CMD_LEN]);                      
int printHistory(char command[MAX_CMD][MAX_CMD_LEN]);  

int WithOutput(char buf[BUFFSIZE]);          
int WithInput(char buf[BUFFSIZE]);          
int WithReOutput(char buf[BUFFSIZE]);        
int WithPipe(char buf[BUFFSIZE]);            
int InBackground(char buf[BUFFSIZE]);

int get_input(char buf[]) {
    memset(buf, 0x00, BUFFSIZE);
    memset(backupBuf, 0x00, BUFFSIZE);        

    fgets(buf, BUFFSIZE, stdin);
    buf[strlen(buf) - 1] = '\0';
    return strlen(buf);
}
void parse(char* buf) {
    for (i = 0; i < MAX_CMD; i++) {
        argv[i] = NULL;
        for (j = 0; j < MAX_CMD_LEN; j++)
            command[i][j] = '\0';
    }
    argc = 0;
    strcpy(backupBuf, buf);
    int len = strlen(buf);
    for (i = 0, j = 0; i < len; ++i) {
        if (buf[i] != ' ')
            command[argc][j++] = buf[i];
        else {
            if (j != 0) {
                command[argc][j] = '\0';
                ++argc;
                j = 0;
            }
        }
    }
    if (j != 0) 
        command[argc][j] = '\0';
    argc = 0;
    int flg = 0;
    for (i = 0; buf[i] != '\0'; i++) {
        if (flg == 0 && !isspace(buf[i])) {
            flg = 1;
            argv[argc++] = buf + i;
        } 
        else if (flg == 1 && isspace(buf[i])) {
            flg = 0;
            buf[i] = '\0';
        }
    }
    argv[argc] = NULL;
}
void do_cmd(int argc, char* argv[]) {
    pid_t pid;
    int pipefd[2];
    for (j = 0; j < MAX_CMD; j++) {
        if (strcmp(command[j], ">") == 0) {
            strcpy(buf, backupBuf);
            WithOutput(buf);
        }
        if (strcmp(command[i], "<") == 0) {
            strcpy(buf, backupBuf);
            WithInput(buf);
        }
        if (strcmp(command[j], ">>") == 0) {
            strcpy(buf, backupBuf);
            WithReOutput(buf);
        }
        if (strcmp(command[j], "|") == 0) {
            strcpy(buf, backupBuf);
            WithPipe(buf);
        }
    }
    for (j = 0; j < MAX_CMD; j++) {
        if (strcmp(command[j], "&") == 0) {
            strcpy(buf, backupBuf);
            InBackground(buf);
        }
    }
    if (strcmp(command[0], "cd") == 0){
        getcwd(curPath, BUFFSIZE);
        printf("curPath:%s\n",curPath);
        Cd(command);
    }
    else if (strcmp(command[0], "history") == 0) 
        printHistory(command);
    else if (strcmp(command[0], "exit") == 0) 
        exit(0);
    else {
        switch(pid = fork()) { 
            case -1:
                printf("创建子进程未成功");
                return;
            case 0:
                execvp(argv[0], argv);
                exit(1);
            default: {
                    int status;
                    waitpid(pid, &status, 0);      // 等待子进程返回
                    int err = WEXITSTATUS(status); // 读取子进程的返回码            
            }
        }
    }
}
void Cd(char command[MAX_CMD][MAX_CMD_LEN]) {
    static char prevPath[BUFFSIZE];
    char curPath[BUFFSIZE];
    getcwd(curPath, BUFFSIZE);
    if (strcmp(command[1], "") == 0 || strcmp(command[1], "~") == 0) 
        chdir("/home/zxc") != 0;
    else if (strcmp(command[1], "-") == 0) {
        if (strlen(prevPath) > 0) {
            if (chdir(prevPath) != 0) 
                printf("Error\n");
            else {
                printf("%s\n", prevPath);
                strcpy(prevPath, curPath);
            }
        }
    }
    else {
        if (chdir(command[1]) != 0) 
            printf("Error: \n");
        else 
            strcpy(prevPath, curPath);
    }
}
int printHistory(char command[MAX_CMD][MAX_CMD_LEN]) {
    int n = atoi(command[1]);
    for (i = n; i > 0 && commandNum - i >= 0; i--) 
        printf("%d\t%s\n", n - i + 1, history[commandNum - i]);
    return 0;
}
int WithOutput(char buf[BUFFSIZE]) {
    strcpy(buf, backupBuf);
    char outFile[BUFFSIZE];
    memset(outFile, 0x00, BUFFSIZE);

    int Num = 0;
    for (i = 0; i + 1 < strlen(buf); i++) {
        if (buf[i] == '>' && buf[i + 1] == ' ') 
            Num++;
    }

    for (i = 0; i < argc; i++) {
        if (strcmp(command[i], ">") == 0) {
            if (i + 1 < argc)
                strcpy(outFile, command[i + 1]);
            else
                return 0;
        }
    }
    // 指令分割, outFile为输出文件, buf为重定向符号前的命令
    for (j = 0; j < strlen(buf); j++) {
        if (buf[j] == '>')
            break;
    }
    buf[j - 1] = '\0';
    buf[j] = '\0';
    // 解析指令
    parse(buf);
    pid_t pid;
    switch(pid = fork()) {
        case -1:
            printf("创建子进程未成功");
            return 0;
        case 0: {
            int fd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, 0777);
            dup2(fd, STDOUT_FILENO); 
            close(fd); 
            execvp(argv[0], argv);
            exit(1);
        }
        default: {
            int status;
            waitpid(pid, &status, 0);       // 等待子进程返回
            int err = WEXITSTATUS(status);  // 读取子进程的返回码
            if (err) 
            printf("子进程执行出错：%s\n", strerror(err));
        }                        
    }

}
int WithInput(char buf[BUFFSIZE]) {
    strcpy(buf, backupBuf);
    char inFile[BUFFSIZE];
    memset(inFile, 0x00, BUFFSIZE);

    int Num = 0;
    for ( i = 0; i + 1< strlen(buf); i++) {
        if (buf[i] == '<' && buf[i + 1] == ' ') 
            Num++;
    }
    for (i = 0; i < argc; i++) {
        if (strcmp(command[i], "<") == 0) {
            if (i + 1 < argc)
                strcpy(inFile, command[i + 1]);
            else 
                return 0;
        }
    }
    
    for (j = 0; j < strlen(buf); j++) {
        if (buf[j] == '<') 
            break;
    }
    buf[j - 1] = '\0';
    buf[j] = '\0';
    parse(buf);
    pid_t pid;
    switch(pid = fork()) {
        case -1:
            printf("创建子进程未成功");
            return 0;
        case 0: {
            int fd = open(inFile, O_RDONLY, 0777);
            dup2(fd, STDIN_FILENO); 
            close(fd);
            execvp(argv[0], argv);
            exit(1);
        }
        default: {
            int status;
            waitpid(pid, &status, 0);       // 等待子进程返回
            int err = WEXITSTATUS(status);  // 读取子进程的返回码
            if (err) 
                printf("Error: %s\n", strerror(err)); 
        }                        
    }

}
int WithReOutput(char buf[BUFFSIZE]) {
    strcpy(buf, backupBuf);
    char reOutFile[BUFFSIZE];
    memset(reOutFile, 0x00, BUFFSIZE);

    int Num = 0;
    for ( i = 0; i + 2 < strlen(buf); i++) {
        if (buf[i] == '>' && buf[i + 1] == '>' && buf[i + 2] == ' ') 
            Num++;
    }

    for (i = 0; i < argc; i++) {
        if (strcmp(command[i], ">>") == 0) {
            if (i + 1 < argc)
                strcpy(reOutFile, command[i + 1]);
            else 
                return 0;
        }
    }

    for (j = 0; j + 2 < strlen(buf); j++) {
        if (buf[j] == '>' && buf[j + 1] == '>' && buf[j + 2] == ' ') 
            break;
    }
    buf[j - 1] = '\0';
    buf[j] = '\0';
    parse(buf);
    pid_t pid;
    switch(pid = fork()) {
        case -1: 
            printf("创建子进程未成功");
            return 0;
        case 0: {
            int fd = open(reOutFile, O_WRONLY|O_APPEND|O_CREAT|O_APPEND, 7777);
            dup2(fd, STDOUT_FILENO); 
            close(fd);
            execvp(argv[0], argv);  
            exit(1);
        }
        default: {
            int status;
            waitpid(pid, &status, 0);       // 等待子进程返回
            int err = WEXITSTATUS(status);  // 读取子进程的返回码
            if (err)  
                printf("Error: %s\n", strerror(err));
        }                        
    }   
}
int WithPipe(char buf[BUFFSIZE]) {
    int count = 0;
    for(j = 0; buf[j] != '\0'; j++) {
        if (buf[j] == '|')
            count++;
    }
    // 分离指令, 将管道符号前后的指令存放在两个数组中
    char outputBuf[BUFFSIZE];
    memset(outputBuf, 0x00,BUFFSIZE);
    char inputBuf[BUFFSIZE];
    memset(inputBuf, 0x00, BUFFSIZE);

    int output_len = 0;
    int input_len = 0;

    for (i = 0; i < j - 1 && buf[i] != '|'; i++) {
        outputBuf[output_len++] = buf[i];
    }

    for (i = i + 2; i < strlen(buf) && buf[i] != '\0'; i++) {
        inputBuf[input_len++] = buf[i];
    }

    int pd[2];
    if (pipe(pd) < 0) 
        exit(1);
    pid_t pid = fork();
    if (pid < 0) 
        exit(1);
    if (pid == 0) {                     // 子进程写管道
        close(pd[0]);                   // 关闭子进程的读端
        dup2(pd[1], STDOUT_FILENO);     // 将子进程的写端作为标准输出
        parse(outputBuf);
        execvp(argv[0], argv);
        if (pd[1] != STDOUT_FILENO) 
            close(pd[1]);
    }
    else {                              // 父进程读管道
        int status;
        waitpid(pid, &status, 0);       // 等待子进程返回
        int err = WEXITSTATUS(status);  // 读取子进程的返回码
        if (err)
            printf("Error: %s\n", strerror(err));

        close(pd[1]);                    // 关闭父进程管道的写端
        dup2(pd[0], STDIN_FILENO);       // 管道读端读到的重定向为标准输入
        parse(inputBuf);
        execvp(argv[0], argv);
        if (pd[0] != STDIN_FILENO) 
            close(pd[0]);    
    }
    return 1;
}
int InBackground(char buf[BUFFSIZE]) {
    char backgroundBuf[strlen(buf)];
    memset(backgroundBuf, 0x00, strlen(buf));
    for (i = 0; i < strlen(buf); i++) {
        backgroundBuf[i] = buf[i];
        if (buf[i] == '&') {
            backgroundBuf[i] = '\0';
            backgroundBuf[i - 1] = '\0';
            break;
        }
    }

    pid_t pid = fork();
    if (pid < 0) 
        exit(1);

    if (pid == 0) {
        // 将stdin、stdout、stderr重定向到/dev/null
        freopen( "/dev/null", "w", stdout );
        freopen( "/dev/null", "r", stdin ); 
        signal(SIGCHLD,SIG_IGN);
        parse(backgroundBuf);
        execvp(argv[0], argv);
        exit(1);
    }
}
int main() {
    signal(SIGINT,SIG_IGN);//忽略contral c;
    signal(SIGHUP,SIG_IGN);//忽略控制终端关闭的信号

    while(1) {
        printf("[my_super_shell]$ ");
        if (get_input(buf) == 0)
            continue;
        strcpy(history[commandNum++], buf);
        strcpy(backupBuf, buf);
        parse(buf);
        do_cmd(argc, argv);
        argc = 0;
    }
}