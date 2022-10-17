#include "headers.h"
#include "utils.h"
#include "functions.h"

char* user,*root,*cwd,*prevwd;
char hostname[1024];
pid_t bgPid[100] = {0};
char* bgCommand[100];
int fgTime = 0,fgRun = 0;
int pipeFd[2][2];

void handler() {
    if (!fgRun) {
        printf("\n");
        prompt(user,hostname,cwd,root,fgTime);
    }
    fgRun = 0;
}

void addBg(char* command,int pid){
    if(!pid)
        return;
    int empty = 99;

    while(bgPid[empty] != 0) empty--;

    bgPid[empty] = pid;
    bgCommand[empty] = malloc(strlen(command));
    strcpy(bgCommand[empty],command);
}

void removeBg(int pid){
    if(!pid)
        return;

    for(int i = 0; i < 100; i++){
        if(bgPid[i] == pid){
            bgPid[i] = 0;
            free(bgCommand[i]);
            break;
        }
    }
}

void bgDeath(){
    int status;
    int pid = waitpid(-1, &status, WNOHANG);

    if(pid > 0){
        int pos = 99;
        while(bgPid[pos] != pid)
            pos--;
        if(WIFSTOPPED(status))
            return;
        printf("\n%s with pid %d exited %s\n",bgCommand[pos],pid,WIFEXITED(status)?"normally":"abnormally");

        prompt(user,hostname,cwd,root,0);

        bgPid[pos] = 0;
        free(bgCommand[pos]);
    }
}

void processCommand(char* command,int input,int output){
    char* token = strtok(command," \n");
    char* comm = (char*)malloc(128);
    comm[0]='\0';
    char* args[50];
    int argCount = 0,background = 0,clength = 0;
    int iMode = 0,oMode = 0;
    int inputFd = input,outputFd = output;
    int stdincpy = 0,stdoutcpy = 0;

    while(token != NULL){
        if(iMode){
            inputFd = open(token,O_RDONLY);
            iMode = 0;
            token = strtok(NULL," \n");
            continue;
        }
        if(oMode){
            outputFd = open(token,O_WRONLY | O_CREAT | (oMode == 1 ? O_TRUNC : O_APPEND),0644);
            oMode = 0;
            token = strtok(NULL," \n");
            continue;
        }
        int l = strlen(token);
        clength += l+1;
        if(token[l-1] == '&'){
            background = 1;
            token[l-1] = '\0';
        }else if(token[l-1] == '<'){
            iMode = 1;
        }else if(!strcmp(token,">")){
            oMode = 1;
        }else if(!strcmp(token,">>")){
            oMode = 2;
        }
        if(token[0] != '\0' && !oMode && !iMode){
            sprintf(comm,"%s%s ",comm,token);
            args[argCount] = token;
            args[argCount+1] = NULL;
            argCount++;
        }
        if(background)
            break;
        token = strtok(NULL," \n");
    }

    if(inputFd != -1){
        stdincpy = dup(0);
        dup2(inputFd,0);
    }
    if(outputFd != -1){
        stdoutcpy = dup(1);
        dup2(outputFd,1);
    }
    
    if(argCount == 0){
        return;
    }if(!strcmp(args[0],"cd")){
        if(argCount > 2){
            printf("Error: Too many arguments\n");
        }else if(cd(cwd,args[1],root,prevwd) == -1){
            perror("Error");
        }
    }else if(!strcmp(args[0],"pwd")){
        if(argCount > 1){
            printf("Error: Too many arguments\n");
        }else
            pwd(cwd);
    }else if(!strcmp(args[0],"echo")){
        echo(args);
    }else if(!strcmp(args[0],"ls")){
        ls(cwd,args,root);
    }else if(!strcmp(args[0],"exit")){
        printf("Exited with code 0\n");
        exit(0);
    }else if(!strcmp(args[0],"pinfo")){
        if(argCount > 2)
            printf("Error: Too many arguments\n");
        else
            pinfo(args[1],bgPid);
    }else if(!strcmp(args[0],"discover")){
        if(argCount > 5)
            printf("Error: Too many arguments\n");
        else
            discover(args,cwd,root);
    }else if(!strcmp(args[0],"history")){
        if(argCount > 1)
            printf("Error: Too many arguments\n");
        else
            history(root);
    }else if(!strcmp(args[0],"jobs")){
        if(argCount > 2)
            printf("Error: Too many arguments\n");
        else{
            int w = 0;
            if(argCount > 1 && args[1][0] == '-'){
                if(args[1][1] == 'r')
                    w = 1;
                else if(args[1][1] == 's')
                    w = 2;
            }
            jobs(w,bgPid,bgCommand);
        }
    }else if(!strcmp(args[0],"sig")){
        if(argCount > 3)
            printf("Error: Too many arguments\n");
        else if(argCount < 3)
            printf("Error: Too few arguments\n");
        else
            sig(bgPid,bgCommand,args);
    }else if(!strcmp(args[0],"fg")){
        if(argCount > 2)
            printf("Error: Too many arguments\n");
        else if(argCount < 2)
            printf("Error: Too few arguments\n");
        else{
            int pid = fg(bgPid,args[1]);
            if(pid > 0)
                removeBg(pid);
        }
    }else if(!strcmp(args[0],"bg")){
        if(argCount > 2)
            printf("Error: Too many arguments\n");
        else if(argCount < 2)
            printf("Error: Too few arguments\n");
        else
            bg(bgPid,args[1]);
    }else{
        if(!bg)
            fgRun = 1;
        int pid = runCommand(args,background,&fgTime);
        if(pid > 0)
            addBg(comm,pid);
        fgRun = 0;
    }
    if(inputFd != -1){
        close(inputFd);
        dup2(stdincpy,0);
        close(stdincpy);
    }
    if(outputFd != -1){
        close(outputFd);
        dup2(stdoutcpy,1);
        close(stdoutcpy);
    }
    if(background){
        processCommand(command+clength,input,output);
    }
}

int main(){
    struct passwd *p = getpwuid(getuid());
    user = p ->pw_name;
    gethostname(hostname,1024);

    cwd = (char*)malloc(1024*sizeof(char));
    prevwd = (char*)malloc(1024*sizeof(char));
    root = (char*)malloc(1024*sizeof(char));

    signal(SIGCHLD, bgDeath);
    signal(SIGINT, handler);
    signal(SIGTSTP, handler);

    getcwd(root,1024);
    getcwd(prevwd,1024);

    while(1){
        getcwd(cwd,1024);
        prompt(user,hostname,cwd,root,fgTime);
        fgTime = 0;

        char** commands[20];
        getCommands(commands,fgRun,hostname,user,cwd,root,fgTime);

        for(int i = 0; commands[i] != NULL; i++){
            int pipeCount = 0;
            while(commands[i][pipeCount] != NULL) pipeCount++;
            int input = -1,output = -1;
            for(int j = 0; commands[i][j] != NULL; j++){
                if(pipeCount > 1 && j < pipeCount -1){
                    if(pipe(pipeFd[j%2]) < 0){
                        perror("Error");
                        break;
                    }
                    output = pipeFd[j%2][1];
                }
                if(pipeCount > 1 && j > 0){
                    input = pipeFd[1 - j%2][0];
                }
                processCommand(commands[i][j],input,output);
            }
        }
        fflush(stdout);
    }
}