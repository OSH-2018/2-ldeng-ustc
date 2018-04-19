#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define APPENDFILE 2
#define USEFILE 1
#define USESTD 0

int isValid(char c);
void splitCmd(char cmd[], char *args[], int *num, int cmdpos[], int *inType, char infile[], int *outType, char outFile[]);
void myexec(char *const args[], int runtype);   //runtype = 1时，才对外部命令进行fork（）操作,runtype==0时和普通exec无异

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];

    int bkstdinfd = dup(STDIN_FILENO);      //控制台输入备份
    int bkstdoutfd = dup(STDOUT_FILENO);    //控制台输出备份

    while (1) {
        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);

        int num;
        int cmdpos[128];
        int inType;
        int outType;
        char infile[128];
        char outfile[128];

        splitCmd(cmd, args, &num, cmdpos, &inType, infile, &outType, outfile);

        /* 没有输入命令 */
        if (num == 0){
            continue;
        }
        else if(num == 1){
            if(inType == USEFILE){
                int infilefd = open(infile, O_RDONLY);
                dup2(infilefd, STDIN_FILENO);
            }

            if(outType == USEFILE){
                int outfilefd = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
                dup2(outfilefd, STDOUT_FILENO);
            }
            else if(outType == APPENDFILE){
                int outfilefd = open(outfile, O_WRONLY|O_CREAT|O_APPEND, 0664);
                dup2(outfilefd, STDOUT_FILENO);
            }

            myexec(args, 1);    //内建命令直接在主进程中运行，外部命令在新进程中运行
            dup2(bkstdinfd, STDIN_FILENO);
            dup2(bkstdoutfd, STDOUT_FILENO);
        }
        else{
            int infilefd;
            int outfilefd;
            if(inType == USEFILE){
                infilefd = open(infile, O_RDONLY);
            }
            else{
                infilefd = STDIN_FILENO;
            }

            if(outType == USEFILE){
                outfilefd = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
            }
            else if(outType == APPENDFILE){
                outfilefd = open(outfile, O_WRONLY|O_CREAT|O_APPEND, 0664);
            }
            else{
                outfilefd = STDOUT_FILENO;
            }

            //所有命令皆在新进程中进行。
            int pipefds[128][2];
            int i;
            for(int i=0; i<num; i++){
                pipe(pipefds[i]);
                int pid = fork();
                if(pid == 0){   //子进程运行第i个命令
                    if(i == 0){
                        dup2(infilefd ,STDIN_FILENO);
                    }
                    else{
                        close(pipefds[i-1][1]);
                        dup2(pipefds[i-1][0], STDIN_FILENO);
                        close(pipefds[i-1][0]);
                    }
                    if(i == num-1){
                        dup2(outfilefd, STDOUT_FILENO);
                    }
                    else{
                        close(pipefds[i][0]);
                        dup2(pipefds[i][1], STDOUT_FILENO);
                        close(pipefds[i][1]);
                    }
                    myexec(args + cmdpos[i], 0);
                    exit(255);
                }
                else{
                    if(i != 0){
                        close(pipefds[i-1][0]);
                    }
                    close(pipefds[i][1]);
                }
                wait(NULL);
            }
            //wait(NULL); //父进程
            dup2(bkstdinfd, STDIN_FILENO);
            dup2(bkstdoutfd, STDOUT_FILENO);
        }
    }
}

int isValid(char c){
    return c!='|' && c!='\n' && c!='<' && c!='>' && c!='\0';
}

void splitCmd(char cmd[], char *args[], int *num, int cmdpos[], int *intype, char infile[], int *outtype, char outfile[]){

    *intype = *outtype = USESTD;
    infile[0] = outfile[0] = '\0';

    char newcmd[256];

    int i=-1;
    int k=0;
    while(isspace(cmd[++i]));    //去除前端空格
    for(; cmd[i]!='\n';){    //分离输入输出文件，去掉输入输出文件的部分在newcmd中。

        if(cmd[i]=='>'){
            if(cmd[i+1] != '>'){    //使用文件
                *outtype = USEFILE;
                i++;
            }
            else{                   //在文件中追加
                *outtype = APPENDFILE;
                i += 2;
            }
            int j=0;
            while(isValid(cmd[i]))
            {
                if(!isspace(cmd[i])){
                    outfile[j++] = cmd[i];
                }
                i++;
            }
            outfile[j] = '\0';
        }
        else if(cmd[i]=='<'){
            *intype = USEFILE;
            i++;
            int j=0;
            while(isValid(cmd[i])){
                if(!isspace(cmd[i])){
                    infile[j++] = cmd[i];
                }
                i++;
            }
            infile[j] = '\0';
        }
        else{
            newcmd[k++] = cmd[i++];
        }
    }
    newcmd[k] = '\0';
    while(newcmd[k-1] == ' ')  //去除末尾空格
        newcmd[--k] = '\0';

    memcpy(cmd, newcmd, sizeof(char)*256);   //只剩下管道部分

    k=0;
    for(i=0; cmd[i]!='\0'; i++){
        if( cmd[i]=='|'){    //去除管道符后的空格
            newcmd[k++] = cmd[i];
            while(isspace(cmd[++i]));
        }
        else if( isspace(cmd[i]) && (isspace(cmd[i+1])||cmd[i+1]=='|') ){ //去除连续空格或管道符前的空格
            continue;
        }
        newcmd[k++] = cmd[i];
    }
    newcmd[k] = '\0';

    memcpy(cmd, newcmd, sizeof(char)*256);   //管道两端无空格的标准模式

    int cnt = 0;    //当前用到的args
    *num = 0;
    args[cnt] = cmd;
    if(isValid(cmd[0]))
        cmdpos[(*num)++] = cnt++;


    for(i=0; cmd[i]!='\0'; i++){
        if(isspace(cmd[i])){
            cmd[i] = '\0';
            args[cnt++] = cmd + (i+1);
        }
        else if(cmd[i]=='|'){
            cmd[i] = '\0';
            args[cnt++] = NULL;
            args[cnt] = cmd + (i+1);
            if(isValid(cmd[i+1]))
                cmdpos[(*num)++] = cnt++;
        }
    }
    args[cnt] = NULL;


    return;

}

void myexec(char *const args[], int runtype){
    /* 内建命令 */
    if (strcmp(args[0], "cd") == 0) {
        if (args[1])
            chdir(args[1]);
    }
    else if (strcmp(args[0], "pwd") == 0) {
        char wd[4096];
        puts(getcwd(wd, 4096));
    }
    else if (strcmp(args[0], "exit") == 0)
        exit(0);
    else if (strcmp(args[0], "export") == 0){
        int i=1;
        while(args[i]){
            int j;
            char name[128];
            char value[128];
            for(j=0;args[i][j]!='=';j++)
                name[j] = args[i][j];
            name[j] = '\0';
            int k=0;
            j++;
            for(;args[i][j]!='\0';j++)
                value[k++] = args[i][j];
            value[k] = '\0';
            setenv(name,value,1);
            i++;
        }
    }
    else{
        int pid = runtype==1 ? fork() : 0;      //只在runtype==1时使用新进程
        if(pid == 0){
            execvp(args[0], args);
            exit(255);
        }
        else{
            wait(NULL);
            return;
        }
    }
    if(runtype == 0){   //runtype==0和普通exec无异，内建命令也要返回
        exit(0);
    }
}

/*

    ls 1     2  3|  cd 4 5   6   |wc 7  8 9


*/

