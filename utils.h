#ifndef UTILS_H
#define UTILS_H

typedef struct File{
    char perms[11];
    char* name;
    char lastMod[25];
    int links;
    off_t size;
    char* user;
    char* group;
    int colour;
    int len,linklen;
} file;

void getCommands(char*** commands,int fgRun,char* hostname,char* user, char* cwd,char* root,int fgTime);
void processPath(char* root,char newPath[],char* path);
int cmp(const void* a, const void* b);
int cmp2(const void* a, const void* b);
void printContents(char* dir,int all,int details);
int getPerms(char *perms,int mode);
int numLength(int x);
int runCommand(char* args[],int bg,int* fgTime);
void find(char* path,char* home,int d,int f,char file[]);
int getHistory(char* root,char** history);
void addHistory(char* root,char* args);
int checkNumber(char* str);
void longestCommonPrefix(char* x,char* y);
int autocomplete(char* complete,char* hostname,char* user, char* cwd,char* root,int fgTime);

#endif