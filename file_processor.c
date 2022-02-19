#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>

struct targs {
  sem_t *sem; /* semaphore */
  char *shm; /* shared memory */ 
  size_t shm_sz; /* size of shared memory */
};

char* getLineInput(){
    char line[1000];
    fgets(line, 1000, stdin);
    return line;
}

void getInput(char* outLines){
    for(int i = 0; i < 50; i++){
        char* line = getLineInput();
        if(strncmp(line, "STOP\n", 5) == 0){
            i = 50;
        }
        else strncat(outLines, line, strlen(line));
    }
}

void replaceNewline(char* lines){
    char* currPosition = strchr(lines, '\n');
    while (currPosition){
        *currPosition = ' ';
        currPosition = strchr(lines, '\n');
    }
}

void replaceplusplus(char* lines){
    char lines2[strlen(lines)];
    strcpy(lines2, lines);
    char* currPosition = strstr(lines, "++");
    while (currPosition){
        for(int i = 0; i < strlen(lines2); i++){
            if(lines2[i] == '+' && lines2[i+1] == '+'){
                lines2[i] = '%';
                lines2[i+1] = 's';
                sprintf(lines, lines2, "^");
                strcpy(lines2, lines);
            }
        }
        currPosition = strstr(lines, "++");
    }
}

char **formatLines(char* lines, char* output[]){
    int alive = 1;
    int linePos = 0;
    int rowPos = 0;
    while(alive){
        if(linePos + 80 < strlen(lines))
            output[rowPos] = calloc(80, sizeof(char));
        else output[rowPos] = calloc(strlen(lines)-linePos, sizeof(char));
        for(int i = 0; i < 80; i++){
            if(linePos < strlen(lines)){
                output[rowPos][i] = lines[linePos];
                linePos++;
            }
            else {
                alive = 0;
                break;
            }
        }
        rowPos++;
    }
    return output;
}

void inputFile(char* inputF, char* input){
    int source;
    // open scource file
    source = open(inputF, O_RDONLY);
    if (source == -1) { 
        printf("cannot open %s for input\n", inputF); 
        exit(1); 
    }
    // duplicate source file to STDIN
    int result = dup2(source, 0);
    if (result == -1) { 
        perror("source dup2()"); 
        exit(2); 
    }
    close(source);
}

void outputFile(char* outputF, char* input){
    int source;
    // open scource file
    source = open(outputF, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (source == -1) { 
        perror("source open()");
        exit(1); 
    }
    // duplicate source file to STDOUT
    int result = dup2(source, 1);
    if (result == -1) { 
        perror("source dup2()"); 
        exit(2); 
    }
    close(source);
}



int main(int argc, char *argv[]) {

    char inLines[50000];
    char outLines[625][80];
    if( argc >= 3){
        if(strncmp(argv[1], "<", 1) == 0)
            inputFile(argv[2], inLines);
        else if(strncmp(argv[1], ">", 1) == 0)
            outputFile(argv[2], inLines);
        if(argc >= 5){
            if(strncmp(argv[3], "<", 1) == 0)
                inputFile(argv[4], inLines);
            else if(strncmp(argv[3], ">", 1) == 0)
                outputFile(argv[4], inLines);
        }
            
    }
    
    getInput(inLines);
    
    replaceNewline(inLines);
    replaceplusplus(inLines);
    char **printout = formatLines(inLines, outLines);
    
    
    
    for(int i = 0; i<625; i++){
        if(strlen(printout[i]) == 80)
            printf("%s\n", printout[i]);
        else {
            printf("%s\n", printout[i]);
            break;
        }
    }
    
}