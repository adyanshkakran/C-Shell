#include "headers.h"
#include "utils.h"
#include "functions.h"

struct termios orig_termios;

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        perror("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) perror("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) perror("tcsetattr");
}

void longestCommonPrefix(char* x,char* y){
    int len = strlen(x) > strlen(y) ? strlen(y) : strlen(x);
    int comm = 0;
    for(int i = 0; i < len; i++){
        if(x[i] == y[i])
            comm++;
        else
            break;
    }
    x[comm] = '\0';
}

int autocomplete(char* complete,char* hostname,char* user, char* cwd,char* root,int fgTime){
    if(complete[0] == '\0'){
        struct dirent* d;
        DIR *dh = opendir(cwd);
        if(!dh){
            perror("Error");
            return 0;
        }
        printf("\n");
        while((d = readdir(dh)) != NULL){
            if(strcmp(d->d_name,".") && strcmp(d->d_name,"..")){
                printf("%s",d->d_name);
                if(d->d_type == DT_DIR)
                    printf("/");
                printf("\n");
            }
        }
        prompt(user,hostname,cwd,root,fgTime);
        return 0;
    }
    int matches = 0,len = strlen(complete);
    char* options[150] = {NULL};
    int isdir[150];
    char* path = (char*)malloc(150);
    char* last = (char*)malloc(150);
    char* buffer = (char*)malloc(1024);
    strcpy(buffer,complete);
    
    char* sep = strtok(buffer," ");

    while(sep != NULL){
        strcpy(last,sep);
        sep = strtok(NULL," ");
    }
    if(last[0] == '~'){
        sprintf(path,"%s/%s",root,last+2);
    }else if(last[0] == '/'){
        strcpy(path,last);
    }else{
        sprintf(path,"%s/%s",cwd,last);
    }

    strcpy(buffer,path);
    sep = strtok(buffer,"/");
    while(sep != NULL){
        strcpy(last,sep);
        sep = strtok(NULL,"/");
    }
    if(complete[len-1] == '/'){
        last[0] = '\0';
    }
    int lastlen = strlen(last);
    path[strlen(path)-lastlen] = '\0';

    struct dirent* d;
    DIR *dh = opendir(path);

    if(!dh){
        perror("\nError");
        printf("\n");
        complete[0] = '\0';
        prompt(user,hostname,cwd,root,fgTime);
        return 0;
    }

    while((d = readdir(dh)) != NULL){
        if(strlen(d->d_name) < lastlen || !strcmp(d->d_name,".") || !strcmp(d->d_name,".."))
            continue;
        int match = 1;
        for (int i = 0; i < lastlen; i++){
            if(d->d_name[i] != last[i]){
                match = 0;
                break;
            }
        }
        if(match){
            options[matches] = (char*)malloc(strlen(d->d_name));
            strcpy(options[matches],d->d_name);
            isdir[matches++] = (d->d_type == DT_DIR);
        }
    }

    if(matches == 1){
        for(int i = 0; i < len; i++)
            printf("\b \b");
        complete[len-lastlen] = '\0';
        if(isdir[0])
            sprintf(complete,"%s%s/",complete,options[0]);
        else
            sprintf(complete,"%s%s ",complete,options[0]);
        printf("%s",complete);
    }else if(matches > 1){
        printf("\n");
        complete[len-lastlen] = '\0';
        char* pref = (char*)malloc(150);
        strcpy(pref,options[0]);
        for(int i = 0; i < matches; i++){
            printf("%s",options[i]);
            if(i > 0)
                longestCommonPrefix(pref,options[i]);
            if(isdir[i])
                printf("/");
            printf("\n");
        }
        strcat(complete,pref);
        prompt(user,hostname,cwd,root,fgTime);
        printf("%s",complete);
    }
    return matches;
}

void getCommands(char*** commands,int fgRun,char* hostname,char* user, char* cwd,char* root,int fgTime){
    char* history[21] = {NULL};
    int n = getHistory(root,history);
    char* complete = (char*)malloc(1024);
    complete[0] = '\0';
    char c;
    int len = 0,hist = n;
    
    enableRawMode();
    while(read(STDIN_FILENO, &c, 1) == 1){
        if(c == 4){ // CTRL + D
            printf("\n");
            exit(0);
        }else if(c == '\n'){
            complete[len++] = '\n';
            complete[len] = '\0';
            printf("\n");
            break;
        }else if (c == 27) { // ARROW
            char buf[3];
            buf[2] = 0;
            if (read(STDIN_FILENO, buf, 2) == 2) {
                if(buf[1] == 'A' && hist > 0){
                    hist--;
                    for (int i = 0; i < strlen(complete); i++)
                        printf("\b \b");
                    strcpy(complete,history[hist]);
                    len = strlen(complete)-1;
                    complete[len] = '\0';
                    printf("%s",complete);
                }else if(buf[1] == 'B' && hist < n-1){
                    hist++;
                    for (int i = 0; i < strlen(complete); i++)
                        printf("\b \b");
                    strcpy(complete,history[hist]);
                    len = strlen(complete)-1;
                    complete[len] = '\0';
                    printf("%s",complete);
                }
            }
        }else if (c == 127) {       // BACKSPACE
            if (len > 0) {
                complete[--len] = '\0';
                printf("\b \b");
            }
        }else if (c == 9){          // TAB
            int m = autocomplete(complete,hostname,user,cwd,root,fgTime);
            len = strlen(complete);
        }else{
            complete[len++] = c;
            complete[len] = '\0';
            printf("%c",c);
        }
        fflush(stdout);
    }
    disableRawMode();
    complete[len] = '\0';

    addHistory(root,complete);

    char* semiTokens[64];
    char* token = strtok(complete,";");
    int comm = 0;

    commands[0] = NULL;

    for(int i = 0; token != NULL; i++){
        semiTokens[i] = token;
        semiTokens[i+1] = NULL;
        token = strtok(NULL,";");
    }

    while(semiTokens[comm] != NULL){
        commands[comm+1] = NULL;
        commands[comm] = malloc(40 * sizeof(char*));
        commands[comm][0] = NULL;
        int c = 0;
        char* cpy = semiTokens[comm];
        char* t = strtok(cpy,"|");

        while(t != NULL){
            commands[comm][c++] = t;
            commands[comm][c] = NULL;
            t = strtok(NULL,"|");
        }

        comm++;
    }
}

void processPath(char* root,char newPath[],char* path){
    for(int i = 0; ; i++){
        if(root[i] == '\0'){
            newPath[0] = '~';
            break;
        }else if( path[i] == '\0' || root[i] != path[i])
            break;
    }

    if(newPath[0] == '~'){
        strcat(newPath,path + strlen(root));
    }else{
        strcpy(newPath,path);
    }
}

void printContents(char* dir,int all,int details){
    file files[128];
    int f = 0,maxLength = 0,maxLinks = 0;
    struct dirent* d;
    DIR *dh = opendir(dir);

    if(!dh){
        perror("Error");
        return;
    }
    while((d = readdir(dh)) != NULL){
        if(!all & d->d_name[0] == '.')
            continue;
        files[f].name = d->d_name;
        files[f+1].name = NULL;
        char* filePath = (char*)malloc(512);
        sprintf(filePath,"%s/%s",dir,files[f].name);

        struct stat st;
        stat(filePath,&st);

        files[f].links = st.st_nlink;
        files[f].user = getpwuid(st.st_uid)->pw_name;
        files[f].group = getgrgid(st.st_gid)->gr_name;
        files[f].size = st.st_size;

        files[f].len = numLength(files[f].size);
        if(files[f].len > maxLength)
            maxLength = files[f].len;
        int linklen = numLength(files[f].links);
        if(linklen > maxLinks)
            maxLinks = linklen;

        strftime(files[f].lastMod,25,"%b %d %H:%M",gmtime(&(st.st_mtim.tv_sec)));
        if(files[f].lastMod[4] == '0')
            files[f].lastMod[4] = ' ';
        files[f].colour = getPerms(files[f].perms,st.st_mode);
        f++;
    }
    
    qsort(files,f,sizeof(file),cmp);
    for(int i = 0; i < f; i++){
        if(details){
            printf("%s ",files[i].perms);
            for(int j = 0; j < maxLinks-numLength(files[i].links); j++)
                printf(" ");
            printf("%d %s %s ",files[i].links,files[i].user,files[i].group);
            for(int j = 0; j < maxLength-files[i].len; j++)
                printf(" ");
            printf("%ld %s ",files[i].size,files[i].lastMod);
        }
        if(files[i].colour == 0)
            printf("\033[0;32m%s\033[0m  ",files[i].name);
        else if(files[i].colour == 1)
            printf("\033[0;34m%s\033[0m  ",files[i].name);
        else 
            printf("\033[1;31m%s\033[0m  ",files[i].name);
        if(details)
            printf("\n");
    }
    if(details)
        printf("\n");
    if(!details)
        printf("\n");
}

int cmp(const void* a, const void* b){
    file* aa = (file*)a;
    file* bb = (file*)b;
    return strcmp((aa->name), (bb->name));
}

int cmp2(const void* a, const void* b){
    process* aa = (process*)a;
    process* bb = (process*)b;
    return strcmp((aa->name), (bb->name));
}

int numLength(int x){
    if(x == 0)
        return 1;
    int len = 0;
    while(x > 0){
        x /= 10;
        len++;
    }
    return len;
}

int getPerms(char *perms,int mode){
    int col = 0;
    for(int i = 0; i < 10; i++)
        perms[i] = '-';
    if(mode & S_IRUSR)
        perms[1] = 'r';
    if(mode & S_IWUSR)
        perms[2] = 'w';
    if(mode & S_IXUSR){
        perms[3] = 'x';
        col = 2;
    }
    if(mode & S_IRGRP)
        perms[4] = 'r';
    if(mode & S_IWGRP)
        perms[5] = 'w';
    if(mode & S_IXGRP){
        perms[6] = 'x';
        col = 2;
    }
    if(mode & S_IROTH)
        perms[7] = 'r';
    if(mode & S_IWOTH)
        perms[8] = 'w';
    if(mode & S_IXOTH){
        perms[9] = 'x';
        col = 2;
    }
    if(S_ISDIR(mode)){
        perms[0] = 'd';
        col = 1;
    }
    perms[10] = '\0';
    return col;
}

int runCommand(char* args[],int bg,int* fgTime){
    char path[64];
    int pid;
    sprintf(path,"/usr/bin/%s",args[0]);
    FILE* file = fopen(path,"r");
    if(file){ // Checking if the given command exists or not
        pid = fork();
        fclose(file);
    }else{
        printf("Error: Command Not Found\n");
        return 0;
    }
    if(pid < 0){
        perror("Error");
        return -1;
    }else if(!pid){
        setpgid(0,0);

        if(execvp(path,args) < 0){
            perror("Error");
            return -1;
        }
    }else{
        if(bg){
            printf("%s process started with pid: %d\n",args[0],pid);
            return pid;
        }else{
            int status;
            signal(SIGTTIN, SIG_IGN);
            signal(SIGTTOU, SIG_IGN);

            tcsetpgrp(0, pid);

            clock_t t;
            t = time(NULL);

            waitpid(pid, &status, WUNTRACED);

            t = time(NULL) - t;
            *fgTime = t;

            tcsetpgrp(0, getpgid(0));

            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            if (WIFSTOPPED(status)) return pid; // Ctrl+Z

            if (WEXITSTATUS(status) == EXIT_FAILURE) return -1;            
        }
    }
}

void find(char* path,char* home,int d,int f,char file[]){
    struct dirent* di;
    DIR *dh = opendir(path);

    if(!dh){
        perror("Error");
        return;
    }
    while((di = readdir(dh)) != NULL){
        if(!strcmp(di->d_name,"..") || !strcmp(di->d_name,"."))
            continue;
        if(d == 0){
            if(file[0] != '#'){
                if(!strcmp(file,di->d_name)){
                    printf("%s/%s\n",home,di->d_name);
                }
            }else if(f == 0){
                printf("%s/%s\n",home,di->d_name);
            }else{
                if(di->d_type != DT_DIR)
                    printf("%s/%s\n",home,di->d_name);
            }
        }else{
            if(file[0] != '#'){
                if(!strcmp(file,di->d_name))
                    printf("%s/%s\n",home,di->d_name);
            }else if(f == 1){
                printf("%s/%s\n",home,di->d_name);
            }else{
                if(di->d_type == DT_DIR)
                    printf("%s/%s\n",home,di->d_name);
            }
        }

        if(di->d_type == DT_DIR){
            char newPath[260],newHome[260];
            sprintf(newPath,"%s/%s",path,di->d_name);
            sprintf(newHome,"%s/%s",home,di->d_name);
            find(newPath,newHome,d,f,file);
        }
    }
}

int getHistory(char* root,char** history){
    char path[128];
    size_t size = 1024;
    int num = 0;
    sprintf(path,"%s/.history",root);
    FILE* f = fopen(path,"a");fclose(f); // Just in case it doesn't exist
    FILE* fp = fopen(path,"r");

    if(fp){
        for(int i = 0; i < 20; i++){
            history[i+1] = NULL;
            if(getdelim(&history[i],&size,'\n',fp) > 0)
                num++;
            else
                break;
        }
        fclose(fp);
    }
    return num;
}

void addHistory(char* root,char* command){
    int valid = 0;

    for (int i = 0; command[i] != '\0' && !valid; i++) {
        if (command[i] != ' ' && command[i] != '\t' && command[i] != '\n') valid = 1;
    }

    if (!valid) return;

    char path[128];
    sprintf(path,"%s/.history",root);

    char* history[21] = {NULL};
    int n = getHistory(root,history);

    if(n == 0 || strcmp(command,history[n-1]) != 0){
        FILE* f = fopen(path,"w");
        for(int i = (n==20?1:0); i < n; i++){
            fprintf(f,"%s",history[i]);
        }
        fprintf(f,"%s",command);
        fclose(f);
    }
}

int checkNumber(char* str){
    for(int i = 0; str[i] != '\0'; i++){
        if(str[i] < '0' || str[i] > '9')
            return 0;
    }
    return 1;
}