#include "headers.h"
#include "utils.h"
#include "functions.h"

void prompt(char* user,char* hostname,char* cwd,char* root,int time){
    char path[1024] = " ";
    processPath(root,path,cwd);
    if(time > 1){
        printf("\033[0;32m%s@%s\033[0m: \033[0;36m%s\033[0m took %ds>",user,hostname,path,time);
        return;
    }
    printf("\033[0;32m%s@%s\033[0m: \033[0;36m%s\033[0m >",user,hostname,path);
    fflush(stdout);
}

int cd(char* cwd,char* path,char* root,char* prevwd){
    char* newPath = (char*)malloc(1024);

    if(path != NULL && path[0] == '-'){ // cd -
        if(chdir(prevwd) == -1)
            return -1;
        processPath(root,newPath,prevwd);
        printf("%s\n",newPath);
        strcpy(newPath,prevwd);
        strcpy(prevwd,cwd);
        strcpy(cwd,newPath);
        return 0;
    }

    strcpy(prevwd,cwd);
    if(path == NULL){ // Only cd

        if(chdir(root) == -1)
            return -1;
        strcpy(cwd,root);

    }else if(path[0] == '~'){  // Relative path

        sprintf(newPath,"%s/%s",root,path+2);
        if(chdir(newPath) == -1)
            return -1;
        strcpy(cwd,newPath);

    }else if(path[0] == '/'){ // Absolute path

        if(chdir(path) == -1)
            return -1;
        strcpy(cwd,path);
        
    }else{

        sprintf(newPath,"%s/%s",cwd,path);
        if(chdir(newPath) == -1)
            return -1;
        strcat(cwd,"/");
        strcat(cwd,path);

    }
}

void pwd(char* cwd){
    printf("%s\n",cwd);
}

void echo(char* args[]){
    for(int i = 1; args[i] != NULL; i++){
        printf("%s ", args[i]);
    }
    printf("\n");
}

void ls(char* cwd,char* args[],char* root){
    int all = 0;
    int details = 0;
    int arguments = 0;

    // Flags
    for(int i = 1; args[i] != NULL; i++){
        if(args[i][0] == '-'){
            if(args[i][1] == 'a' || args[i][2] == 'a')
                all = 1;
            if(args[i][1] == 'l' || args[i][2] == 'l')
                details = 1;
        }else
            arguments++;
    }

    
    if(arguments == 0){
        printContents(cwd,all,details);
    }else{
        for(int i = 1; args[i] != NULL; i++){
            if(args[i][0] != '-'){
                if(args[i][0] == '~'){
                    char path[512];
                    sprintf(path,"%s/%s",root,args[i]+2);
                    printContents(path,all,details);
                }else if(args[i][0] == '/'){
                    printContents(args[i],all,details);
                }else{
                    char path[512];
                    sprintf(path,"%s/%s",cwd,args[i]);
                    printContents(path,all,details);
                }
            }
        }
    }
}

void pinfo(char* process,int bgPid[100]){
    int pid;
    if(process == NULL)
        pid = getpid();
    else
        pid = atoi(process);
    char path[20];
    sprintf(path,"/proc/%d/stat",pid);
    FILE* fd = fopen(path,"r");
    if(!fd){
        printf("Error: Process with pid %d not found\n",pid);
        return;
    }
    int memory;
    int bg = 0;
    char state = 'R';
    fscanf(fd,
           "%*d %*s %c %*s %*d %*s %*s %*d %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %d",
           &state, &memory);
    fclose(fd);
    for (int i = 0; i < 100; i++){
        if(bgPid[i] == pid){
            bg = 1;break;
        }
    }
    char exec[128];
    sprintf(path,"/proc/%d/exe",pid);
    realpath(path,exec);

    printf("pid: %d\n",pid);
    printf("Process Status: %c%s\n",state,(bg==1?"":"+"));
    printf("Memory: %d {Virtual memory}\n",memory);
    printf("Executable path: %s\n",exec);
}

void discover(char* args[],char* cwd,char* root){
    int directories = 0;
    int files = 0,path = 0;
    char dir[128],file[64];
    dir[0] = '#';
    file[0] = '#';

    // Argument handling
    for(int i = 1; args[i] != NULL; i++){
        if(args[i][0] == '-'){
            if(args[i][1] == 'd' || args[i][2] == 'd')
                directories = 1;
            if(args[i][1] == 'f' || args[i][2] == 'f')
                files = 1;
        }else if(args[i][0] == '"'){
            args[i][strlen(args[i])-1] = '\0';
            strcpy(file,args[i]+1);
        }else{
            if(args[i][0] == '~')
                args[i][0] = '.';
            if(args[i][0] == '.'){
                if(strlen(args[i]) == 1)
                    sprintf(dir,"%s",root);
                else
                    sprintf(dir,"%s/%s",root,args[i]+2);
            }else if(args[i][0] == '/'){
                strcpy(dir,args[i]);
            }else{
                sprintf(dir,"%s/%s",cwd,args[i]);
            }
            path = i;
        }
    }
    if(dir[0] != '#'){
        if(directories && file[0] == '#')
            printf("%s\n",args[path]);
        find(dir,args[path],directories,files,file);
    }else{
        if(directories && file[0] == '#')
            printf(".\n");
        find(cwd,".",directories,files,file);
    }
}

void history(char* root){
    char *history[21] = {NULL};
    int n = getHistory(root,history);

    for(int i = (n>10?n-10:0); i < n; i++)
        printf("%s",history[i]);
}

void jobs(int which,int* bgPid,char **bgCommand){
    char processPath[128];
    process jobs[128];
    int num = 0;

    for (int i = 99; i >= 0; i--){
        if(bgPid[i] == 0) continue;

        sprintf(processPath,"/proc/%d/stat",bgPid[i]);
        FILE* f = fopen(processPath,"r");
        if(!f)
            continue;
        fscanf(f,"%*d %*s %c",&jobs[num].status);
        if(which == 0 || (which == 1 && jobs[num].status != 'T') || (which == 2 && jobs[num].status == 'T')){
            jobs[num].id = 100-i;
            jobs[num].name = (char*)malloc(strlen(bgCommand[i]));
            strcpy(jobs[num++].name,bgCommand[i]);
        }
        fclose(f);
    }
    qsort(jobs,num,sizeof(process),cmp2);
    for(int i = 0; i < num; i++)
        printf("[%d] %s %s [%d]\n",jobs[i].id,(jobs[i].status == 'T'?"Stopped":"Running"),jobs[i].name,bgPid[100-jobs[i].id]);
}

void sig(int* bgPid,char** bgCommand,char **args){
    if(!checkNumber(args[1]) || !checkNumber(args[2])){
        printf("Error: Process Id should be a number\n");
        return; 
    }
    int index = 100-atoi(args[1]),signal = atoi(args[2]);
    if(index < 0 || index > 99 || bgPid[index] == 0){
        printf("Error: Process Does Not Exist\n");
        return;
    }
    if(kill(bgPid[index],signal) == -1)
        perror("Error");
}

int fg(int* bgPid,char *process){
    if(!checkNumber(process)){
        printf("Error: Process Id should be a number\n");
        return -1;
    }
    int index = 100 - atoi(process);
    if(index < 0 || index > 99 || bgPid[index] == 0){
        printf("Error: Process Does Not Exist\n");
        return -1;
    }
    int status;
    setpgid(bgPid[index],getpgid(0));
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(0, bgPid[index]);

    if (kill(bgPid[index], SIGCONT) < 0){ 
        perror("Error");
        return 0;
    }

    waitpid(bgPid[index], &status, WUNTRACED);
    tcsetpgrp(0, getpgid(0));

    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    if (WIFSTOPPED(status)) return 0;

    return bgPid[index];
}

void bg(int* bgPid,char* process){
    if(!checkNumber(process)){
        printf("Error: Process Id should be a number\n");
        return;
    }
    int index = 100 - atoi(process);
    if(index < 0 || index > 99 || bgPid[index] == 0){
        printf("Error: Process Does Not Exist\n");
        return;
    }
    if(kill(bgPid[index],SIGCONT) < 0)
        perror("Error");
}