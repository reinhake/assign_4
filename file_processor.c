#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

struct targs {
  sem_t *sem; /* semaphore */
  char *shm; /* shared memory */ 
  size_t shm_sz; /* size of shared memory */
};

char* getLineInput(){
    char line[1000];
    fgets(line, 2048, stdin);
    return line;
}

void getInput(char* outLines){
    for(int i = 0; i < 5; i++){
        char* line = getLineInput();
        strncat(outLines, line, strlen(line));
    }
}

void replaceNewline(char* lines){
    char* currPosition = strchr(lines, '\n');
    while (currPosition){
        *currPosition = ' ';
        currPosition = strchr(lines, '\n');
    }
}

int main(int argc, char *argv[]) {
    char outLines[50000];
    getInput(outLines);
    replaceNewline(outLines);
    printf("%s", outLines);
}