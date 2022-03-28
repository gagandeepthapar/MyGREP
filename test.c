
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
#include <signal.h>

int *myWait;
int fd[2];
char path[PATH_MAX];
const char *new = "\n";
typedef struct timespec timespec;

void writeTime(int pipe){
    write(pipe, "yes", 4);
    return;
}

int main(){

    // fprintf(stderr, "NEW%d\n", strlen(new));

    myWait = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,0,0);
    *myWait = 0;

    timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    sleep(1);
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);

    uint64_t delT = ((now.tv_sec - start.tv_sec )*1000000) + ((now.tv_nsec - start.tv_nsec)/1000);
    
    int ms = delT/1000;

    int seconds = ms/1000;

    int hr = (int)(seconds/3600);
    int min = ((int)(seconds/60))%60;
    int sec = (int)(seconds%60);
    int millis = ms%1000;

    char txt[100];
    sprintf(txt, "%.2d:%.2d:%.2d:%.3d", hr, min, sec, millis);

    fprintf(stderr, "%s\n", txt);



    return 0;

}