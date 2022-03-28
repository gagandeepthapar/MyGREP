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

// definitions
#define MAXPROC 10
#define MAXSTR 100
typedef struct timespec timespec;
const char *her = "Helping Parent";
const char *scout = "Searching for query";
const char *killer = "Killing Child(ren)";

char *targets;

typedef struct child{

    pid_t SN; // serial number (PID)
    timespec birth; // time of creation
    pid_t parent;   // who created this child
    char path[PATH_MAX];    // current path
    char *process;    // declares its job (see jobs below);

} child;

typedef struct sharemem{
    child *processList;
    int *processCount;
    pid_t parent;

} sharemem;

sharemem globalmem;

// general
void createChild(char *proc){
    if(*(globalmem.processCount) == MAXPROC){
        fprintf(stderr, "No More Children Allowed\n");
        return 0;
    }

    pid_t chld = fork();
    if(chld > 0){
        return;
    }

    globalmem.processList[*(globalmem.processCount)].SN = getpid();
    time(&(globalmem.processList[*(globalmem.processCount)].birth));
    globalmem.processList[*(globalmem.processCount)].parent = getppid();
    globalmem.processList[*(globalmem.processCount)].process = proc;
    getcwd((globalmem.processList[*(globalmem.processCount)].path), PATH_MAX);

    *(globalmem.processCount) = *(globalmem.processCount) + 1;
    return;

}

void removeChild(pid_t SN){

}
// find file
void findWord(char *word, int sub, int ext){

    fprintf(stderr, "find \"%s\": %d %d\n", word, sub, ext);

}

void findFile(char *word, int sub, int ext){

    fprintf(stderr, "find \"%s\"/: %d %d\n", word, sub, ext);

}

// find word

// list
void listChild(){

    fprintf(stderr, "COUNT: %d\n", *(globalmem.processCount));
    fprintf(stderr, "0\t%d\tPARENT\n", (globalmem.parent));
    fprintf(stderr, "-\t------\t-------\n");
    fprintf(stderr, "#\tSerial\tProcess\n");
    fprintf(stderr, "-\t------\t-------\n");

    for(int i = 0; i < *(globalmem.processCount); i++){
        fprintf(stderr,"%d\t%d\t%s\n", i+1, globalmem.processList[i].SN, (globalmem.processList[i].process));
    }

}

// kill

// quit

// input
void printPrompt(){

    fprintf(stderr, "\033[0;34m");
    fprintf(stderr, "findstuff.c");
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "$ ");

}

void parseFind(){

    if(strlen(targets) < 6){
        fprintf(stderr, "invalid query\n");
        return 1;
    }

    int sub = 0, ext = 0;

    if((strstr(targets, "-f:") != NULL)){
        ext = 1;
    }

    if((strstr(targets, "-s") != NULL)){
        sub = 1;
    }

    char word[10];

    int i = 5;
    do{

        if(targets[i] == ' '){
            break;
        }
        word[i-5] = targets[i];
        i = i+1;

    }while(targets[i] != ' ');
    word[i-5] = '\0';

    fprintf(stderr, "TARGET: %s\n",word);

    if(targets[5] == '\"'){
        findWord(word, sub,ext);
        return 0;
    }

    findFile(word, sub,ext);
    return 0;
}

int checkFind(){

    char find[] = "find";
    for(int i = 0; i < strlen(find); i++){
        if(targets[i] != find[i]){
            return 0;
        }

        return 1;
    }

}

int checkList(){
    if(strcmp(targets, "list") == 0){
        return 1;
    }
    return 0;
}

int checkKill(){

    char kill[] = "kill";
    // fprintf(stderr, "%d\n", strlen(inp));
    if(strlen(targets) < 6 || strlen(targets) > 7){
        return 0;
    }

    for(int i = 0; i < strlen(kill); i++){
        if(targets[i] != kill[i]){
            return 0;
        }
    }

    for(int i = 0; i < strlen(targets); i++){
        fprintf(stderr, "%c\n", targets[i]);
    }

    fprintf(stderr, "5: %c\n6:%c\nSTRLEN: %d\n", targets[5], targets[6], strlen(targets));

    int ret;

    if(strlen(targets) == 6){
        ret = ((int) targets[5] - 48);
    }
    else{
        ret = ((int) targets[5] - 48) * 10;
        ret = ret + ((int) targets[6] - 48);
    }

    if(ret > (globalmem.processCount[0])){
        fprintf(stderr, "Exceeds Number of Children\n");
        return 0;
    }

    return ret;
}

int checkQuit(){

    if(targets[0] == 'q'){
        return 1;
    }
    return 0;
}

int readIn(){
    printPrompt();

    // char store[MAXSTR];
    fgets(targets, MAXSTR, stdin);
    targets[strlen(targets)-1] = '\0';

    if(checkList()){
        fprintf(stderr, "listing children\n");
        return 0;
    }
    
    if(checkFind()){
        fprintf(stderr, "check find\n");
        parseFind();
        return 0;
    }

    if(checkKill()){
        fprintf(stderr, "kill child\n");
        return 0;
    }

    if(checkQuit()){
        fprintf(stderr, "quit all\n");
        return 0;
    }

    fprintf(stderr, "invalid input\n");
    return 1;

}

int main(){
    // initialize shared/malloc'd memory
    globalmem.processCount = mmap(NULL, (MAXPROC+1)*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    globalmem.processList = mmap(NULL, MAXPROC*sizeof(child), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    // targets = mmap(NULL, MAXSTR * sizeof(char), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    targets = (char *)malloc(MAXSTR);
    globalmem.parent = getpid();
    globalmem.processCount[0] = 0;

    while(1){
        readIn();
    }

    return 0;
}