#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>//pid_t，为无符号整型
#include <sys/wait.h> //waitpid()
/*
管道符 | 是用于连接两个命令，将第一个命令的输出作为第二个命令的输入的特殊符号。
实现的原理如下：
当你使用管道符时，Shell 将会创建一个管道(pipe)，它是一个特殊的文件描述符，可以连接两个进程
第一个命令的标准输出被重定向到管道的写端
第二个命令的标准输入被重定向到管道的读端
这样，第一个命令的输出就会成为第二个命令的输入，从而实现了两个命令之间的数据传输
步骤：
1.解析用户输入的命令，识别出命令和参数。
2.创建管道。
3.创建子进程来执行第一个命令，并将输出重定向到管道的写入端。
4.创建另一个子进程来执行第二个命令，并将输入重定向到管道的读取端。
5.父进程关闭管道的两个端，并等待子进程执行完毕。
*/
int main(){
    char *cmd1[] = {"ls", "-l", NULL};
    //cmd1[0] 是要执行的程序的文件路径，cmd1 则是一个字符串数组，包含了执行程序的参数列表，以及一个以 NULL 结尾的标记数组结束符
    char *cmd2[] = {"grep", "txt", NULL};
    int pfd[2];
    //成功返回0，否则返回-1 
    //参数数组包含pipe使用的两个文件的描述符。fd[0]:读管道，fd[1]:写管道。
    if(pipe(pfd) == -1){
        perror("pfd");
        exit(EXIT_FAILURE);
    }
    //必须在fork()中调用pipe()，否则子进程不会继承文件描述符
    //两个进程不共享祖先进程，就不能使用pipe。但是可以使用命名管道
    pid_t pid1 = fork();
    switch(pid1){
        case -1://失败：返回 -1。
            perror("process");
            exit(EXIT_FAILURE);
        case 0: //成功：子进程中返回 0，父进程中返回子进程 ID
            //关闭管道读端
            close(pfd[0]);
            //dup2()函数可以复制一个文件描述符，并将其副本指定为另一个文件描述符的副本，同时关闭原文件描述符
            //将管道的写端（pfd[1]）复制到标准输出的文件描述符（STDOUT_FILENO
            //将子进程的标准输出重定向到管道写端
            dup2(pfd[1],STDOUT_FILENO);
            //关闭多余的管道写端
            close(pfd[1]);
            //execvp()函数会在当前进程中加载并执行指定的可执行文件，它会取代当前进程的内存映像，因此后续代码不会被执行
            execvp(cmd1[0],cmd1);
            perror("execvp1");
            exit(EXIT_FAILURE);
    }
    pid_t pid2 = fork();
    switch(pid2){
        case -1:
            perror("process2");
            exit(EXIT_FAILURE);
        case 0: 
            //关闭管道写端
            close(pfd[1]);
            //将标准输入重定向到管道读端
            dup2(pfd[0],STDIN_FILENO);
            //关闭多余的管道读端
            close(pfd[0]);
            execvp(cmd2[0],cmd2);
            perror("execvp2");
            exit(EXIT_FAILURE);
    }
    //父进程关闭管道两端，并等待子进程执行完毕
    close(pfd[0]);//关闭管道的读端，表示父进程不再需要从管道中读取数据
    close(pfd[1]);//关闭管道的写端，表示父进程不再需要向管道中写入数据
    //waitpid()用于等待指定的子进程终止或者获取子进程的状态信息
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    return 0;
}