// Gagandeep Thapar; CPE 357 Program 4

// include libs
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
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

    pid_t serial;
    pid_t parent;
    timespec start;
    char input[PATH_MAX];
    char target[PATH_MAX];
    char *action;
    int sub;
    int ext;
    int word;
    char extChars[10];

}child;

typedef struct sharemem{

    child *list;
    int *count;
    pid_t *parent;
    timespec *timerStart;
    timespec *progStart;

} sharemem;

static char *path;

typedef struct signalset{

    int *myWait;
    int *listSig;
    int *waitChild;

} signalset;

typedef struct pipeset{

    int list[2];
    int path[2];
    int query[2];
    int timing[2];

} pipeset;

sharemem globalmem;
signalset sigs;
pipeset pipes;

const char *scout = "Searching for query";
const char *clipboard = "Assembling list";
const char *manager = "Managing search queries";
const char *frontDesk = "Reading and parsing inputs";
const char *nothing = "query not found ";
const char *found = "\033[0;32mfound query in:\033[0m\n";

char *input;
char *temp;
char *target;

// standard prompt
void prompt(){
    fprintf(stderr, "\033[0;34m");
    fprintf(stderr, "findstuff.c:");
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "$ ");
}

// print time diff
void printTime(timespec *start){

    // prints and formats time diff
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    uint64_t delT = ((now.tv_sec - start->tv_sec )*1000000) + ((now.tv_nsec - start->tv_nsec)/1000);
    
    int ms = delT/1000;

    int seconds = ms/1000;

    int hr = (int)(seconds/3600);
    int min = ((int)(seconds/60))%60;
    int sec = (int)(seconds%60);
    int millis = ms%1000;

    fprintf(stderr, "%.2d:%.2d:%.2d:%.3d", hr, min, sec, millis);

}

void writeTime(timespec *start){

    // creates string to write to pipe

    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    uint64_t delT = ((now.tv_sec - start->tv_sec )*1000000) + ((now.tv_nsec - start->tv_nsec)/1000);
    
    int ms = delT/1000;

    int seconds = ms/1000;

    int hr = (int)(seconds/3600);
    int min = ((int)(seconds/60))%60;
    int sec = (int)(seconds%60);
    int millis = ms%1000;

    char time[15];
    sprintf(time, "%.2d:%.2d:%.2d:%.3d", hr, min, sec, millis);

    write(pipes.timing[1], time, 15);

    return;

}
// munmap all
void munmapAll(){

    munmap(globalmem.list, MAXPROC*sizeof(child));
    munmap(globalmem.count, (MAXPROC+1)* sizeof(int));
    munmap(globalmem.parent, sizeof(pid_t));
    munmap(globalmem.timerStart, sizeof(timespec));
    munmap(globalmem.progStart, sizeof(timespec));

    munmap(path, PATH_MAX* sizeof(char));
    
    munmap(sigs.myWait, sizeof(int));
    munmap(sigs.listSig, sizeof(int));
    munmap(sigs.waitChild, sizeof(int));

    munmap(input, PATH_MAX*sizeof(char));
    munmap(temp, PATH_MAX * sizeof(char));
    munmap(target, PATH_MAX* sizeof(char));

}

// remove child
void removeChild(int idx){

    // dont kill chld 1
    if(idx == 1){
        fprintf(stderr, "\ncannot kill child 1 by design\n\n");
        *(sigs.waitChild) = 1;
        return;
    }

    int t = 0, i = 0;

    // while going through list
    while(i < MAXPROC){
        if(globalmem.count[i+1] == 1){
            t += 1; // increment number of active children
        }

        // once the idx has been found
        if(t == idx){
            globalmem.count[i+1] = 0;
            globalmem.count[0] -= 1;
            kill(globalmem.list[i].serial, 5);  // tell to exit
            break;
        }

        i += 1;

    }

    return;

}

// setting extension if flag was given
void setExt(child *me){
    
    int i = me->ext + 3;
    while(i < strlen(me->input)){   // given start of flag
        me->extChars[i - (me->ext +3)] = me->input[i];  // write to array
        i += 1;
        if(me->input[i] == ' '){ break; }
    }  
    me->extChars[i] = '\0';

}

//signal for child to exit
int exitSig(int sig){
    *(sigs.waitChild) = 1;
    exit(0);
    return;
}

// returns int in list
int findMe(int pid){

    // finds index of child in list
    int i = 1;
    while(globalmem.list[i-1].serial != pid){
        i += 1;
    }

    return i;

}

int getExt(char *file, child *me){

    char ext[10];

    int i = 0, j = -1;
    while(i < strlen(file)){
        if(file[i] == '.'){ // traverse string until '.' is found
            j = 0;  // set j to on
        }

        i+= 1;

        if(file[i] == ' '){ break; }    // if space is found then quit

        if(j >= 0){ // while j is on
            ext[j] = file[i];   // write extension characters
            j += 1;
        }
    }
    
    ext[j] = '\0';

    int ret = !(strcmp(ext, me->extChars)); // check if extensions match
    memset(ext, 0, sizeof(ext));
    return ret;

}

// read file for word
int readFile(char *file, char *target){
    char filedata;
    long filesize;
    char *filestr;

    FILE *f;
    f = fopen(file, "rb");  // opens file
    fseek(f , 0L,SEEK_END); // go to end
    filesize = ftell(f);    // get pointer to end (filesize)

    if(filesize == 0){  // if size was 0 (empty file)
        return 0;   // quit
    }

    rewind(f);  // go back to start

    filestr = calloc(1, filesize+1);    // calloc space for file data
    if(!filestr){   // if failure then return
        fclose(f);
        return -1;
    }

    if( 1!=fread(filestr, filesize, 1 ,f)){ // if can't read then quit
        fclose(f);
        free(filestr);
        return -1;
    }
    
    fclose(f);  // close file

    char *chk = strstr(filestr, target);    // check for substring

    free(filestr);  // free allocated memory

    if(chk != NULL){    // if the pointer is not null then substring was found
        return 1;
    }

    return 0;

}

// search for word
void searchWord(int pid, int foundFlag){

    child *current = &(globalmem.list[pid-1]);

    DIR *dir = opendir(".");
    char curPath[PATH_MAX];
    getcwd(curPath, PATH_MAX);  // store current path

    struct dirent *entry;
    struct stat file;

    int retval;
    int sig = 0;

    while((entry = readdir(dir))!= NULL){


        lstat(entry->d_name, &file);

        if((file.st_mode & S_IFMT) == S_IFDIR){ // skip directories
           continue;
        }

        // if extension was set then check against current file
        if(current->ext != -1){
            if(getExt(entry->d_name, current) == 0){
                continue;
            }
        }

        // readFile and check for substring
        if(readFile(entry->d_name, current->target)){
            // write to pipes if strng found
            if(sig == 0){
                sig = 1;
                if(foundFlag){
                    write(pipes.path[1], found, strlen(found));
                }
                write(pipes.path[1], curPath, strlen(curPath));
                write(pipes.path[1], "\n", 1);
            }
            write(pipes.list[1], entry->d_name, strlen(entry->d_name));
            if(current->sub != -1){
                write(pipes.list[1], " (", 2);
                write(pipes.list[1], curPath, strlen(curPath));
                write(pipes.list[1], ")", 1);
            }
            write(pipes.list[1], "\n", 1);
        }

    }

    // close and reopen dir
    closedir(dir);
    DIR *dir2 = opendir(".");

    // if subdir flag was set then recursively check directories
    if(current->sub != -1){
        while((entry = readdir(dir2)) != NULL){
            lstat(entry->d_name, &file);
            if((file.st_mode & S_IFMT) == S_IFDIR){
                if((strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))){
                    
                    char newPath[strlen(entry->d_name)+2];
                    newPath[0] = '.';
                    newPath[1] = '/';
                    strcpy(newPath+2, entry->d_name);
                    chdir(newPath);
                    searchWord(pid, 0);
                    chdir(path);
                    memset(newPath, 0, sizeof(newPath));

                }
            }
        }
    }

    // close dir
    closedir(dir2);

    if(strcmp(curPath, path) != 0){
        return;
    }

    // write queries/timing
    write(pipes.query[1], current->target, MAXSTR);

    if(sig == 0){
        write(pipes.path[1], nothing,strlen(nothing));
        write(pipes.list[1], "NA\n", 3);
    }

    writeTime(&(current->start));
    kill(current->parent, 7);   // send signal
    removeChild(pid);   // kill child

    return;

}

// search for file
void searchFile(int pid, int sig){

    child *current = &(globalmem.list[pid-1]);

    DIR *dir = opendir(".");

    // stores current working path
    char curPath[PATH_MAX];
    getcwd(curPath, sizeof(curPath));

    struct dirent *entry;
    struct stat file;

    // cycles through files (ONLY FILES)
    while((entry = readdir(dir)) != NULL){

        lstat(entry->d_name, &file);

        // skip directories;
        if((file.st_mode & S_IFMT) == S_IFDIR){
            continue;
        }

        // if extension flag is set
        if(current->ext != -1){
            // check extension
            if(getExt(entry->d_name, current) == 0){
                // if ext doesn't match then skip file
                continue;
            }
        }

        // if this file is the target then write to pipe
        if(strcmp(current->target, entry->d_name) == 0){
            if(sig == 0){
                write(pipes.path[1], found, strlen(found));
                sig = 1;
            }
            write(pipes.path[1], curPath, strlen(curPath));
            write(pipes.path[1], "\n", 1);
        }

    }

    // close dir (and reopen)
    closedir(dir);
    DIR *dir2 = opendir(".");

    // check if subdirectory flag was set
    if(current->sub != -1){
        // if it was then read through directories
        while((entry = readdir(dir2)) != NULL){
            lstat(entry->d_name, &file);

            if((file.st_mode & S_IFMT) == S_IFDIR){
                // enter directory IF it's not the current or prev directory
                if((strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))){
                    char newPath[strlen(entry->d_name)+2];
                    newPath[0] = '.';
                    newPath[1] = '/';
                    strcpy(newPath+2, entry->d_name);
                    chdir(newPath);
                    searchFile(pid, 1); // recursive call
                    chdir(path);    // change path back to here
                    memset(newPath, 0, sizeof(newPath));
                }
            }
        }
    }

    closedir(dir2); // close dir

    // if we were on a diff path than main path then return here
    if(strcmp(curPath, path) != 0){
        return;
    }

    // if we're back to base case then write to pipes
    write(pipes.query[1], current->target, MAXSTR);

    if(sig == 0){
        write(pipes.path[1], nothing,strlen(nothing));
    }

    writeTime(&(current->start));

    // send signal to print output
    kill(current->parent, 6);

    // kill this child
    removeChild(pid);

    return;

}

// find stuff
void scoutQuery(int pid){

    child *current = &(globalmem.list[pid-1]);

    // if file -> searchFiles()
    if(current->word == 0){
        searchFile(pid,0);
        return;
    }

    // else searchWords()
    else{
        searchWord(pid, 1);
        return;
    }
    

}

// list child
void listchild(){

    // writes list of children (and parent)
    timespec now;
    time(&now);

    fprintf(stderr, "\n0) PARENT (%d)\n", *(globalmem.parent));
    fprintf(stderr, "\tUp-Time: ");
    printTime(&(globalmem.progStart));
    fprintf(stderr, "\n\tRole: Running Program\n");

    fprintf(stderr, "\n# Of Children: %d\n", *(globalmem.count));

    for(int i = 0; i < globalmem.count[0]; i++){

        if(globalmem.count[i+1] == 1){

            fprintf(stderr, "\n%d) Serial# %d\n", i+1, globalmem.list[i].serial);
            fprintf(stderr, "\tUp-Time: ");
            printTime(&(globalmem.list[i].start));
            fprintf(stderr, "\n\tJob: %s", globalmem.list[i].action);
            if(strcmp(globalmem.list[i].action, scout) == 0){
                fprintf(stderr, ": %s", globalmem.list[i].target);
            }
            fprintf(stderr, "\n");
            
        }
    }

    return;

}

// create child
int createChild(char *role){

    // check for max children
    int *count = globalmem.count;
    if(count[0] == MAXPROC){
        fprintf(stderr, "\nNo More Children Allowed\n\n");
        return -1;
    }

    // fork
    pid_t chld = fork();
    if(chld > 0){
        // parent should wait here until child is done being made to prevent errors
        while(*(sigs.waitChild) == 0);
        *(sigs.waitChild) = 0;
        return;
    }

    int i = 1;

    while(i < MAXPROC){

        // sets the flag to active in the child array
        if(globalmem.count[i] == 0){
            globalmem.count[i] = 1;
            break;
        }

        i = i+1;
    }

    // creates new child in the allocated array
    child *new = &(globalmem.list[i-1]);
    new->serial = getpid();
    new->parent = getppid();
    clock_gettime(CLOCK_MONOTONIC_RAW, &(new->start));
    new->action = role;

    if(strcmp(role, scout) == 0){   // scouts search for queries

        memset(new->input, 0, MAXSTR);
        strcpy(new->input, input);

        char *subPtr, *extPtr;

        new->sub = -1, new->ext = -1;

        // sets pointer for where -s flag is located
        if((subPtr = strstr(input, "-s"))!=NULL){
            new->sub = subPtr - input;
        }

        // sets flag and determines extension of query
        if((extPtr = strstr(input, "-f:"))!=NULL){
            new->ext = extPtr - input;
            setExt(new);
        }

        int i = 5;
        int offset = 0;
        
        new->word = 0;

        // checks to see if query is a word (or file)
        if(input[5] == '\"'){
            i = i+1;
            offset = 1;
            new->word = 1;
        }

        memset(new->target, 0, MAXSTR);

        do{

            // writes filtered query into target array
            new->target[i-5-offset] = input[i];
            i = i+1;

            if(input[i] == '\"' && offset == 1){
                break;
            }

            if(input[i] == ' ' && offset == 0){
                break;
            }

        } while(i < strlen(input));

        // completes string
        new->target[i] = '\0';

    }
    
    signal(5, exitSig);

    // sets wait signal to open
    globalmem.count[0] += 1;
    *(sigs.waitChild) = 1;

    return i;

}

// main child to read parse and send signals
void parseInput(){
    prompt();

    fgets(input, MAXSTR, stdin);    // get input from cmd line
    input[strlen(input)-1] = '\0';

    if(strcmp(input, "list") == 0){ // list function
        kill(*(globalmem.parent), 1);
        return;
    }

    if(strlen(input) == 1 && input[0] == 'q'){  // quit function
        kill(*(globalmem.parent), 3);
        exit(0);
        return;
    }

    if(strlen(input) < 6){
        fprintf(stderr, "\ninvalid input\n\n");
        return;
    }

    char *killPtr, *findPtr;
    int killIdx, findIdx;

    if((killPtr = strstr(input, "kill")) != NULL){  // kill function

        killIdx = killPtr - input;

        if(killIdx == 0 && strlen(input) > 7){
            fprintf(stderr, "invalid kill command\n");
            return;
        }

        kill(*(globalmem.parent), 4);
        while(*(sigs.waitChild) == 0);
        *(sigs.waitChild) = 0;
        
        return;

    }

    int chld;

    if((findPtr = strstr(input, "find")) != NULL){  // find fnction
        findIdx = findPtr - input;

        if(findIdx == 0){
            chld = createChild(scout);

            if(getpid() != globalmem.list[0].serial){
                scoutQuery(chld);
            }

            return;

        }
    }

    // in last case there was an invalid input
    fprintf(stderr, "\ninvalid input\n\n");
    memset(input, 0, MAXSTR);
    
    return;

}

// signal setting

// signal to kill child
int killSig(int sig){

    strcpy(temp, input);
    int idx;

    // determines which child to kill
    if(strlen(temp) == 6){
        idx = (int) temp[5] - 48;
    }
    else{
        idx = (int) (temp[5] - 48) * 10 + (int) (temp[6] - 48);
    }

    // rejects idx if that many children don't exist
    if(idx > *(globalmem.count)){
        fprintf(stderr, "\nchild does not exist\n\n");
        *(sigs.waitChild) = 1;
        return;
    }

    removeChild(idx);
    return;

}

// signal to change wait flag
int waitSig(int sig){
    *(sigs.myWait) = 1;
    return;
}

// signal to print list of children
int listSig(int sig){
    fprintf(stderr, "\n");
    listchild();
    fprintf(stderr,"\n");
    prompt();
    *(sigs.listSig) = 1;
    return;
}

// signal to quit program
int quitSig(int sig){
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "\nquitting program\n\n");

    for(int i = 0; i < globalmem.count[0]; i++){
        kill(globalmem.list[i].serial, 5);
    }

    // munmap all data
    munmapAll();

    // parent exit
    exit(0);

    return;
}

// signal to print words
int wordList(int sig){

    char txt[PATH_MAX];
    char files[PATH_MAX];
    char time[15];

    // read in pipe data
    read(pipes.path[0], globalmem.list[0].target, PATH_MAX);
    read(pipes.query[0], txt, PATH_MAX);
    read(pipes.list[0], files, PATH_MAX);
    read(pipes.timing[0], time, 15);

    // print pipe data
    fprintf(stderr, "\033[0;32m");
    fprintf(stderr, "query: %s\n", txt);
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "\n%s\n\n\033[0;32mFiles:\n\033[0m", globalmem.list[0].target);
    fprintf(stderr, "%s\n", files);
    fprintf(stderr, "\033[0;32mTIME %s\033[0m\n\n", time);

    // reset memory
    memset(globalmem.list[0].target,0,PATH_MAX);
    memset(txt,0,PATH_MAX);
    memset(files, 0, PATH_MAX);
    memset(time, 0, 15);

    prompt();
    return;
}

// signal to print files
int redirList(int sig){

    // read in pipe data
    char txt[PATH_MAX];
    char time[15];

    read(pipes.path[0], globalmem.list[0].target, PATH_MAX);
    globalmem.list[0].target[strlen(globalmem.list[0].target)-1]= '\0';
    read(pipes.query[0], txt, PATH_MAX);
    read(pipes.timing[0], time, 15);
    
    // print pipe data
    fprintf(stderr, "\033[0;32m");
    fprintf(stderr, "query: %s\n", txt);
    fprintf(stderr, "\033[0m");
    fprintf(stderr, "\n%s\n\n", globalmem.list[0].target);
    fprintf(stderr, "\033[0;32mTIME %s\033[0m\n\n", time);

    // reset data
    memset(globalmem.list[0].target,0,PATH_MAX);
    memset(txt,0,PATH_MAX);
    memset(time, 0, 15);

    prompt();
    return;
}

// setup
void setup(){

    globalmem.list = (child *) mmap(NULL, MAXPROC*sizeof(child), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    globalmem.count = (int *) mmap(NULL, (MAXPROC+1)*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    globalmem.parent = (pid_t *) mmap(NULL, sizeof(pid_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    globalmem.timerStart = (timespec *) mmap(NULL, sizeof(timespec), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    globalmem.progStart = (timespec *) mmap(NULL, sizeof(timespec), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    path = (char *) mmap(NULL, PATH_MAX*sizeof(char), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);

    getcwd(path, PATH_MAX);

    sigs.myWait = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    sigs.listSig = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    sigs.waitChild = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0,0);
    
    input = (char *) mmap(NULL, PATH_MAX*sizeof(char), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    temp = (char *) mmap(NULL, PATH_MAX*sizeof(char), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    target = (char *) mmap(NULL, PATH_MAX*sizeof(char), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);

    pipe(pipes.list);
    pipe(pipes.path);
    pipe(pipes.query);
    pipe(pipes.timing);

    *(globalmem.parent) = getpid();
    clock_gettime(CLOCK_MONOTONIC_RAW, &(globalmem.timerStart));
    clock_gettime(CLOCK_MONOTONIC_RAW, &(globalmem.progStart));
    
    for(int i = 0; i < (MAXPROC+1); i++){
        globalmem.count[i] = 0;
    }

    // set flags
    *(sigs.myWait) = 0;
    *(sigs.listSig) = 0;
    *(sigs.waitChild) = 0;

}

// main
int main(){


    // setup of shared memory
    setup();

    // main child to handle inputs
    createChild(frontDesk);

    // set signals
    if(getpid() == *(globalmem.parent)){
        signal(1, listSig);
        signal(3, quitSig);
        signal(4, killSig);
    }
    if(getpid() == globalmem.list[0].serial){
        signal(6, redirList);
        signal(7, wordList);
    }

    while(1){
        if(getpid() == *(globalmem.parent)){
            
        }

        if(getpid() == globalmem.list[0].serial){
            parseInput();
        }
    }
    return 0;
}