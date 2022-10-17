#ifndef FUNCTIONS_H
#define FUNCTIONS_H

typedef struct Process{
    char* name;
    char status;
    int id;
} process;

int cd(char* cwd,char* path,char* root,char* prevwd);
void pwd(char* cwd);
void echo(char* args[]);
void prompt(char* user,char* hostname,char* cwd,char* root,int time);
void ls(char* cwd,char* args[],char* root);
void pinfo(char* process,int *bgPid);
void discover(char* args[],char* cwd,char* root);
void history(char* root);
void jobs(int which,int* bgPid,char **bgCommand);
void sig(int* bgPid,char** bgCommand,char **args);
int fg(int* bgPid,char *process);
void bg(int* bgPid,char *process);

#endif