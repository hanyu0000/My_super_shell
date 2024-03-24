#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>
#include <signal.h>
#define MAX_CMD_LENGTH 255
#define MAX_PATH_LENGTH 255
#define MAX_BUF_SIZE  4096
#define MAX_ARG_NUM 50
#define MAX_VAR_NUM 50
#define MAX_CMD_NUM 10
#define MAX_VAR_LENGTH 500
#define FORK_ERROR 2
#define EXEC_ERROR 3
struct cmd{
    struct cmd * next;
    int begin,end;  
    int argc;
    char lredir,rredir; 
    char toFile[MAX_PATH_LENGTH],fromFile[MAX_PATH_LENGTH];  
    char *args[MAX_ARG_NUM];
    char bgExec;   
};
struct cmd cmdinfo[MAX_CMD_NUM];
char cmdStr[MAX_CMD_LENGTH];    
int cmdNum,varNum;
char envVar[MAX_VAR_NUM][MAX_PATH_LENGTH];
void Error();
void init();
void setIO();
int  getInput();
int  parseCmds();
int  handleVar();
int  getItem();
int  parseArgs();
int  execInner();
int  execOuter();
int main(){
    signal(SIGINT,SIG_IGN);//忽略contral c;
    signal(SIGHUP,SIG_IGN);//忽略控制终端关闭的信号
    while (1){
        cmdNum = varNum = 0;
        printf("[my_super_shell]$ ");
        fflush(stdin);
        int n = getInput();
        if(n<=0)
        continue;  
        parseCmds(n);
        if(parseArgs()<0)continue;
        for(int i=0;i<cmdNum;++i){
            struct cmd *pcmd=cmdinfo+i, * tmp;
            int status = execInner(pcmd);
            if(status==1){
                pid_t pid = fork();
                if(pid==0)execOuter(pcmd);
                else if(pid<0)Error(FORK_ERROR);
                if(!pcmd->bgExec)wait(NULL);  
                pcmd=pcmd->next; 
                while(pcmd){
                    tmp = pcmd->next;
                    free(pcmd);
                    pcmd=tmp;
                }
            }
        }

    }
    return 0; 
}
void init(struct cmd *pcmd){
    pcmd->bgExec=0;
    pcmd->argc=0;
    pcmd->lredir=pcmd->rredir=0;
    pcmd->next = NULL;
    pcmd->begin=pcmd->end=-1;
    for(int i=0;i<MAX_ARG_NUM;++i)pcmd->args[i]=NULL; 
}
void Error(int n){
    switch(n){
        case FORK_ERROR:printf("fork error\n");break;
        case EXEC_ERROR:printf("exec error\n");break;
		default:printf("Error, exit ...\n");
    }
    exit(1);
}
int getInput(){
    int pCmdStr=0,cur;
    char newline = 1;
    while(newline){
        cur = MAX_CMD_LENGTH-pCmdStr;
        if(cur<=0)
            return -1;
        fgets(cmdStr+pCmdStr,cur,stdin);
        newline = 0;
        while(1){
            if(cmdStr[pCmdStr]=='\\'&&cmdStr[pCmdStr+1]=='\n'){
                newline=1;
                cmdStr[pCmdStr++]='\0';
                break;
            }
            else if(cmdStr[pCmdStr]=='\n')
                break;
            ++pCmdStr;
        }
    }
    return pCmdStr;
}
int  parseCmds(int n){
    char beginCmd=0;
    struct cmd * head;
    for( int i=0;i<=n;++i){
        switch(cmdStr[i]){
            case '&':{
                if(cmdStr[i+1]=='\n'||cmdStr[i+1]==';'){
                    cmdStr[i]=' ';
                    head->bgExec=1;
                }
            }
			case '\t':cmdStr[i]=' ';break;
            case ';':{
                beginCmd = 0;
                cmdStr[i]='\0';  
                cmdinfo[cmdNum++].end=i;
                break;
            }
            case '\n':{
                cmdStr[i]='\0';
                cmdinfo[cmdNum++].end =i;
                return 0;
            }
            case ' ':break;
            default:if(!beginCmd){
                beginCmd=1;
                head = cmdinfo+cmdNum;
                cmdinfo[cmdNum].begin =  i;
            }
        }
    }
}

int getItem(char *dst,char*src, int p){   
    int ct=0;
    while(src[++p]==' ');
    if(src[p]=='\n')return -1; 
    char c;
    while(c=dst[ct]=src[p]){
        if(c==' '||c=='|'||c=='<'||c=='>'||c=='\n')break;
        ++ct,++p;
    }
    dst[ct]='\0';
    return p-1;
}

int handleVar(struct cmd *pcmd,int n){
    char * arg = pcmd->args[n];
    int p_arg=0,p_var=0;
    while(arg[p_arg]){
        if((arg[p_arg]=='$')&&(arg[p_arg-1]!='\\')){
            if(arg[p_arg+1]=='{')p_arg+=2;
            else p_arg+=1;
            char *tmp=&envVar[varNum][p_var];
            int ct=0;
            while(tmp[ct]=arg[p_arg]){
                if(tmp[ct]=='}'){
                    ++p_arg;
                    break;
                }
                if(tmp[ct]==' '||tmp[ct]=='\n'||tmp[ct]=='\0')break;
                ++ct,++p_arg;
            }
            tmp[ct]='\0';
            tmp = getenv(tmp);
            for(int i=0;envVar[varNum][p_var++]=tmp[i++];);
            p_var-=1; 
        }
        else envVar[varNum][p_var++]=arg[p_arg++];
    }
    envVar[varNum][p_var]='\0';
    pcmd->args[n] = envVar[varNum++];
    return 0;
}
int parseArgs(){
    char beginItem=0,beginQuote=0,beginDoubleQuote=0,hasVar=0,c;
	int begin,end;
	struct cmd* pcmd;
    for(int p=0;p<cmdNum;++p){
		if(beginQuote||beginItem||beginDoubleQuote)
			return -1;  
        pcmd=&cmdinfo[p];
        begin = pcmd->begin,end = pcmd->end;
        init(pcmd);
        for(int i=begin;i<end;++i){
            c = cmdStr[i];
			if((c=='\"')&&(cmdStr[i-1]!='\\'&&(!beginQuote))){
				if(beginDoubleQuote){
					cmdStr[i]=beginDoubleQuote=beginItem=0;
                    if(hasVar){
                        hasVar=0;
                        handleVar(pcmd,pcmd->argc-1);  
                    }
                }
                else{
					beginDoubleQuote=1;
					pcmd->args[pcmd->argc++]=cmdStr+i+1;
				}
                continue;
			}
            else if(beginDoubleQuote){
                if((c=='$') &&(cmdStr[i-1]!='\\')&&(!hasVar))hasVar=1;
                continue;
            }
            if((c=='\'')&&(cmdStr[i-1]!='\\')){
                if(beginQuote){
					cmdStr[i]=beginQuote=beginItem=0;
                }
                else{
                    beginQuote=1;
                    pcmd->args[pcmd->argc++]=cmdStr+i+1;
                }
                continue;
            }
            else if(beginQuote) continue;
            if(c=='<'||c=='>'||c=='|'){
                if(beginItem)beginItem=0;
                cmdStr[i]='\0';
            }
            if(c=='<'){
                if(cmdStr[i+1]=='<'){
                    pcmd->lredir+=2;  //<<
                    cmdStr[i+1]=' ';
                }
                else
                    pcmd->lredir+=1;  //<
                int tmp = getItem(pcmd->fromFile,cmdStr,i);
                if(tmp>0)i = tmp;
            }else if(c=='>'){
                if(cmdStr[i+1]=='>'){
                    pcmd->rredir+=2;  //>>
                    cmdStr[i+1]=' ';
                }
                else
                    pcmd->rredir+=1;  //>
                int tmp = getItem(pcmd->toFile,cmdStr,i);
                if(tmp>0)i = tmp;
            }
            else if (c=='|'){
                pcmd->end = i;
                pcmd->next = (struct cmd*)malloc(sizeof(struct cmd));
                pcmd = pcmd->next;
                init(pcmd);
            }
            else if(c==' '||c=='\0'){
                if(beginItem){
                    beginItem=0;
                    cmdStr[i]='\0';
                }
            }
            else{
                if(pcmd->begin==-1)pcmd->begin=i;
                if(!beginItem){
                    beginItem=1;
                    if((c=='$') &&(cmdStr[i-1]!='\\')&&(!hasVar))hasVar=1;
                    pcmd->args[pcmd->argc++]=cmdStr+i;
                }
            }
            if(hasVar){
                hasVar=0;
                handleVar(pcmd,pcmd->argc-1);  
            }
        }
        pcmd->end=end;
    }
}
int execInner(struct cmd* pcmd){  
    if (!pcmd->args[0])
        return 0;
    if (strcmp(pcmd->args[0], "cd") == 0) {
    struct stat st;
    if (pcmd->args[1]) {
        if (!strcmp(pcmd->args[1], "-"))
            chdir("..");
        else if (!strcmp(pcmd->args[1], "~"))
            chdir("home/zxc");
        else {
            stat(pcmd->args[1], &st);
            if (S_ISDIR(st.st_mode))
                chdir(pcmd->args[1]);
            else
                return -1;
        }
    }
    return 0;
} else if (strcmp(pcmd->args[0], "cd") == 0) {
    if (pcmd->args[1] == NULL)
        chdir(getenv("HOME"));
    else if (pcmd->argc == 1)
        chdir(pcmd->args[1]);
    else if (!strcmp("-", pcmd->args[1]))
        chdir("..");
    else if (!strcmp("~", pcmd->args[1]))
        chdir("home/zxc");
    else
        chdir(pcmd->args[1]);
}
    if (strcmp(pcmd->args[0], "pwd") == 0) {
        printf("%s\n",getcwd(pcmd->args[1] , MAX_PATH_LENGTH));
        return 0;
    }
    if (strcmp(pcmd->args[0], "unset") == 0) {
        for(int i=1;i<pcmd->argc;++i)unsetenv(pcmd->args[i]);
        return 0;
    }
    if (strcmp(pcmd->args[0], "export") == 0) {
        for(int i=1;i<pcmd->argc;++i){  
            char *val,*p;
            for(p = pcmd->args[i];*p!='=';++p);
            *p='\0';
            val = p+1;
            setenv(pcmd->args[i],val,1);
        }
        return 0;
    }
    if (strcmp(pcmd->args[0], "exit") == 0)
        exit(0);
    return 1;
} 
    
void setIO(struct cmd *pcmd,int rfd,int wfd){
    if(pcmd->rredir>0){  //  >,  >>
        int flag ;
        if(pcmd->rredir==1)flag=O_WRONLY|O_TRUNC|O_CREAT;  // >  note: trunc is necessary!!!
        else flag=O_WRONLY|O_APPEND|O_CREAT; //>>
        int wport = open(pcmd->toFile,flag);
        dup2(wport,STDOUT_FILENO);
        close(wport);
    }
    if(pcmd->lredir>0){  //<, <<
        int rport  = open(pcmd->fromFile,O_RDONLY);
        dup2(rport,STDIN_FILENO);
        close(rport);
    }
    /* pipe  */
    if(rfd!=STDIN_FILENO){
        dup2(rfd,STDIN_FILENO);
        close(rfd);
    }
    if(wfd!=STDOUT_FILENO){
        dup2(wfd,STDOUT_FILENO);
        close(wfd);
    }
} 
int execOuter(struct cmd * pcmd){
    if(!pcmd->next){
        setIO(pcmd,STDIN_FILENO,STDOUT_FILENO);
        execvp(pcmd->args[0],pcmd->args);
    }
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if(pid<0){
        Error(FORK_ERROR);
    }else if (pid==0){
        close(fd[0]);
        setIO(pcmd,STDIN_FILENO,fd[1]);
        execvp(pcmd->args[0],pcmd->args);
        Error(EXEC_ERROR);
    }else{
        wait(NULL);
        pcmd = pcmd->next;
        close(fd[1]);
        setIO(pcmd,fd[0],STDOUT_FILENO);  
        execOuter(pcmd);
    }
}
struct cmd * reverse(struct cmd* p){
    struct cmd * q=p->next,*r;
    p->next = NULL;
    while(q){
        r = q->next;
        q->next = p;
        p = q;
        q = r;
    }
    return p;
}
int LS(char *path){
    DIR *dirp;
    struct dirent d,*dp = &d;
    dirp  = opendir(path);
    int ct=0;
    while((dp=readdir(dirp))!=NULL){
        printf("%s\n",dp->d_name);
    }
    closedir(dirp);
    return 0;
} 
int CAT(char * path){
    char buf[MAX_BUF_SIZE];
    int fd = open(path,'r'),n;
    while((n=read(fd,buf,MAX_BUF_SIZE))>0){
        write(STDOUT_FILENO,buf,n);
    }
    return 0;
}
int ENV(){
    extern char **environ;
    char **env=environ;
    while(*env){
        printf("%s\n",*(env++));
    }
    return 0;
}