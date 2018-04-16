#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>

#define APPENDFILE 2
#define USEFILE 1
#define USESTD 0

int isValid(char c);
void splitCmd(char cmd[], char *args[], int *num, int cmdpos[], int *inType, char infile[], int *outType, char outFile[]);

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];
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

        printf("in:%s\n",infile);
        printf("out:%s\n",outfile);

        printf("num:%d\n",num);
        int i;
        for(i=0;i<num;i++)
            printf("%d ",cmdpos[i]);
        printf("\n");

        for(i=0; i<num; i++){
            int j;
            printf("command %d (len:%d):\n",i,(int)strlen(args[cmdpos[i]]));
            for(j=cmdpos[i]; args[j]!=NULL; j++){
                printf("%s\n",args[j]);
            }
        }

        /* 没有输入命令 */
        if (!args[0]){
            printf("NO COMMAND\n");
            continue;
        }

        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1])
                chdir(args[1]);
            continue;
        }
        if (strcmp(args[0], "pwd") == 0) {
            char wd[4096];
            puts(getcwd(wd, 4096));
            continue;
        }
        if (strcmp(args[0], "exit") == 0)
            return 0;

        /* 外部命令 */
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程 */
            execvp(args[0], args);
            /* execvp失败 */
            return 255;
        }
        /* 父进程 */
        wait(NULL);
    }
}

int isValid(char c){
    return c!='|' && c!='\n' && c!='<' && c!='>' && c!='\0';
}

void splitCmd(char cmd[], char *args[], int *num, int cmdpos[], int *intype, char infile[], int *outtype, char outfile[]){

    *intype = *outtype = USESTD;

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
                if(isValid(cmd[i])){
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
    while(cmd[k-1] == ' ')  //去除末尾空格
        cmd[--k] = '\0';

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

    printf("new:%s\n",cmd);

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

/*

    ls 1     2  3|  cd 4 5   6   |wc 7  8 9


*/

