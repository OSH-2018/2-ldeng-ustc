#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>

#define USEFILE 1
#define USESTD 0

int isValid(char c);
void splitCmd(char cmd[], int *num, int coms[], int *inType, char infile[], int *outType, char outFile[]);

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
        int coms[128];
        int inType;
        int outType;
        char infile[128];
        char outfile[128];

        splitCmd(cmd,&num,coms,&inType,infile,&outType,outfile);

        printf("in:%s\n",infile);
        printf("out:%s\n",outfile);




        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        /* 拆解命令行 */
        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
                if (*args[i+1] == ' ') {
                    *args[i+1] = '\0';
                    args[i+1]++;
                    break;
                }
        args[i] = NULL;

        /* 没有输入命令 */
        if (!args[0])
            continue;

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

void splitCmd(char cmd[], int *num, int coms[], int *intype, char infile[], int *outtype, char outfile[]){

    *intype = *outtype = USESTD;

    char newcmd[256];

    int i;
    int k=0;
    for(i=0; cmd[i]!='\n';){

        if(cmd[i]=='>')
        {
            *outtype = USEFILE;
            i++;
            int j=0;
            while(isValid(cmd[i]))
            {
                if(!isspace(cmd[i]))
                {
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
            while(isValid(cmd[i]))
            {
                if(isValid(cmd[i]))
                {
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
    return;

}

