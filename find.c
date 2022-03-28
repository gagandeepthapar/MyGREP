// Gagandeep Thapar; CPE 357 Program 4

// include libs
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define MAXSTR 100
#define MAXPROC 10

typedef struct timespec timespec;

typedef struct child{

    pid_t SN;
    pid_t parent;
    timespec starttime;

} child;

typedef struct sharemem{

    child *list;
    int *count;
    pid_t parent;
    timespec start;
    timespec now;

} sharemem;

typedef struct pipeset{

    int listdata[2];
    int pathdata[2];

} pipeset;

char *input;
char *target;
sharemem globalmem;
pipeset pipes;

void createChild(){
    if(globalmem.count[0] == 10){
        fprintf(stderr, "No More Children Allowed\n");
        return 0;
    }

    pid_t chld = fork();
    if(chld > 0){
        sleep(2);
        return;
    }
    else{
        globalmem.count[globalmem.count[0]] = 1;
        globalmem.list[globalmem.count[0]].SN = getpid();
        globalmem.list[globalmem.count[0]].parent = getppid();
        time(&(globalmem.list[globalmem.count[0]].starttime));

        globalmem.count[0] = globalmem.count[0] + 1;

    }
    return;

}

void findFile(int sub, int ext){

    // chdir(path);
    DIR *dir = opendir(".");
    
    char curPath[PATH_MAX];
    getcwd(curPath, sizeof(curPath));
    
    struct stat file;
    struct dirent *entry;
    FILE *fp;
    int sig = 0;

    while((entry = readdir(dir)) != NULL){

        if(strcmp(entry->d_name, target) == 0){
            if(sig == 0){
                fprintf(stderr, "\nFOUND FILE IN:\n");
            }
            fprintf(stderr, "%s\n", curPath);
            sig = 1;
        }

        if(sub == 1){
            fprintf(stderr, "subs on!\n");
            // lstat(entry->d_name, &file);
            // if(file.st_mode & S_IFMT & S_IFDIR){
            //     if(strcmp(entry->d_name, ".")&&strcmp(entry->d_name, "..")){
            //         // fprintf(stderr, "DIRECTORY!\t%s\n", entry->d_name);

            //         char *dir = (char *) malloc(strlen(entry->d_name)+2);
            //         dir[0] = '.';
            //         dir[1] = '/';
            //         strcpy(dir+2, entry->d_name);

            //         // fprintf(stderr, "valid: %s\n", dir);
            //         chdir(dir);
            //         findFile(sub, ext);
            //         free(dir);

            //     }
            // }   
        }

    }

    if(sig == 0){
        fprintf(stderr, "\nNo such file found in %s\n", curPath);
    }

    closedir(dir);
    
    return;

}

void findWord(int sub, int ext){

}

void getWord(int offset){

    int i = 5 + offset;
    do{
        // fprintf(stderr, "%d\t%c\n", i, input[i]);
        target[i - 5 - offset] = input[i];
        i = i+1;

        if(input[i] == ' '){
            break;
        }
        if(input[i] == '\"'){
            break;
        }

    }while(i < strlen(input));

    target[i] = '\0';

    fprintf(stderr, "\ntarget: %s\n\n", target);

}

void listChild(){

    for(int i = 0; i < globalmem.count[0]; i++){
        fprintf(stderr, "%d\t%d\n",i+1, 
        globalmem.list[i].SN);
    }

}

void quitProg(){

}

void killChild(){

}

void find(){

    int sub = 0, ext = 0;

    if(strstr(input, " -s") != NULL){
        sub = 1;
    }

    if(strstr(input, " -f:") != NULL){

        ext = 1;
    }

    if(input[5] == '\"'){
        getWord(1);
        findWord(sub, ext);
        return;
    }
    else{
        getWord(0);

        findFile(sub, ext);
        return;
    }



}

int commandType(){
    if(strcmp(input, "list")==0){
        return 0;
    }

    if(strlen(input) == 1 && input[0] == 'q'){
        return 1;
    }

    if(strlen(input) < 6){
        return -1;
    }
    
    if(input[0] == 'k'
    && input[1] == 'i'
    && input[2] == 'l'
    && input[3] == 'l'){

        if(strlen(input) > 7){
            return -1;
        }

        return 2;
    }

    if(input[0] == 'f'
    && input[1] == 'i'
    && input[2] == 'n'
    && input[3] == 'd'){
        return 3;
    }

}

void printPrompt(){
    fprintf(stderr, "\033[0;34m");
    fprintf(stderr, "findstuff.c");
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "$ ");
}

void getInput(){

    fgets(input, MAXSTR, stdin);
    input[strlen(input)-1] = '\0';

}

int sigHandle(int sig){
    // fprintf(stderr,"\n");
}

int main(){
    input = (char *)mmap(NULL, MAXSTR, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    target = (char *)mmap(NULL, MAXSTR, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    globalmem.list = (child *)mmap(NULL, MAXPROC*sizeof(child), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    globalmem.count = (int *)mmap(NULL, (MAXPROC+1)*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    
    globalmem.parent = getpid();
    globalmem.count[0] = 0;
    time(&(globalmem.start));

    int proc;
    

    while(1){
        if(getpid() == globalmem.list[globalmem.count[0]].SN){
        signal(1, sigHandle);
        printPrompt();
        getInput();
        proc = commandType();

        switch(proc) {
            case 0 :
                listChild();
                break;
            case 1 :
                quitProg();
                break;
            case 2 :
                killChild();
                break;
            case 3 :
                find();
                break;
            default :
                fprintf(stderr, "invalid input\n" );
        }

        memset(input, 0, sizeof(input));
        memset(target, 0, sizeof(input));
        }

        else{

            time(&(globalmem.now));

            while(globalmem.now.tv_sec - globalmem.start.tv_sec < 10){
                time(&(globalmem.now));
            }

        }
    }

}