#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>

//Golabal variable to track when input it finished
volatile sig_atomic_t gEndFunction = 0;

//struct for the different buffers using semaphores
struct targs {
  sem_t *sem; /* semaphore */
  char *shm; /* shared memory */
};

//first buffer with producer getInput and consumer replaceNewLine
struct targs targ1;
sem_t sem1;

//second buffer with producer replaceNewLine and consumer replacePlusPlus
struct targs targ2;
sem_t sem2;

//third buffer with producer replacePlusPlus and consumer formatLines
struct targs targ3;
sem_t sem3;

//helper function for getLineInput that just reads the current input line
void getLineInput(char* line){
    fgets(line, 1000, stdin);
}

//First Thread that cycles through different inputs, stores inputs in the first buffer
//targ1, and then posts it. Checks for STOP\n to end input
void* getInput(){
    for(int i = 0; i < 50; i++){
        char tLine[1000];
        getLineInput(tLine);
        //adds the input from getf into the first buffer
        
        strncat(targ1.shm, tLine, strlen(tLine));
        if(strncmp(tLine, "STOP\n", 5) == 0){
            gEndFunction = 1;
            sem_post(targ1.sem);
            break;
        }
        //increments the first buffer
        sem_post(targ1.sem);
    }
    return NULL;
}

//Second Thread that waits for at least 80 characters to be put in the buffer targ1, and then 
//starts processing and removes all the '\n' and replaces the with spaces before adding
//the processed lines to the second buffer targ2 and posts it
void *replaceNewline(){
    while(1){
        //wait for first buffer to increment
        sem_wait(targ1.sem);
        char* lines = targ1.shm;
        //check to see if 80 chars have been posted
        if(strlen(lines) >= 80 || gEndFunction){
            //changes all \n into spaces
            char* currPosition = strchr(lines, '\n');
            while (currPosition){
                *currPosition = ' ';
                currPosition = strchr(lines, '\n');
            }
            //check if input has ended and add lines to second buffer
            if(gEndFunction)
                strncat(targ2.shm, lines, strlen(lines) - 5);
            else strncat(targ2.shm, lines, strlen(lines));
            //increment the second buffer and then clear the first
            sem_post(targ2.sem);
            strcpy(targ1.shm, "");
        }
    }
    return NULL;
}

//Third thread that waits on input from the buffer targ2 and then processes it by
//replacing ++'s with ^ before adding the process lines into the third buffer targ3
//and posting it.
void *replaceplusplus(){
    while(1){
        //wait for second buffer to increment
        sem_wait(targ2.sem);
        char* lines = targ2.shm;
        
        //search for ++'s and replace them with ^
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
        //copy updated lines to third buffer and increment it,
        //then clear the second buffer
        strncat(targ3.shm, lines, strlen(lines));
        sem_post(targ3.sem);
        strcpy(targ2.shm, "");
    }
    return NULL;
}

//The fourth thread that waits on input from the buffer targ3, it processes input
//by parsing the lines into lines that are 80 long and stores them in the two-d array
//output before then waiting for input to end and printing out the contents of output
void *formatLines(){
    //declare a bunch of counters that will keep track of where in ouput new chars
    //are to be put
    char* output[625];
    int linePos = 0;
    int rowPos = 0;
    int outPos = 0;
    char lines[50000];
    
    while(1){
        //wait for third buffer to increment
        sem_wait(targ3.sem);

        //set up first line of output to be copied to
        strncat(lines, targ3.shm, strlen(targ3.shm));
        if(rowPos == 0)    
            output[rowPos] = calloc(80, sizeof(char));
        int alive = 1;
        
        //while loops that copy each char from the single long string lines
        //putting them into 80 char strings in output
        while(alive){
            //check current output positions to see if it has reached 80 chars
            //if 80 chars have been put on this line print it and set up the next line of output
            if(outPos == 80){
                outPos = 0;
                printf("%s\n", output[rowPos]);
                rowPos++;
                output[rowPos] = calloc(80, sizeof(char));
            }
            //add 80 chars to the currents line of output
            while(outPos < 80){
                if(linePos < strlen(lines)){
                    output[rowPos][outPos] = lines[linePos];
                    linePos++;
                    outPos++;
                }//if there are no new chars to add then break out of the while loops and wait for more
                else {
                    alive = 0;
                    break;
                } 
            }
        }
        //check for input being stopped
        if(gEndFunction){
            exit(0);
        }
        strcpy(targ3.shm, "");
    }
    
    return NULL;
}

//function to open specific inputs 
void inputFile(char* inputF){
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

//function to open specific outputs
void outputFile(char* outputF){
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
    //set up the first buffer to be be 50000 long since the is the maximum allowed input
    targ1.sem = &sem1;
    sem_init(&sem1, 0, 0);
    char buf1[50000];
    targ1.shm = buf1;

    //set up the second buffer to be be 50000 long since the is the maximum allowed input
    targ2.sem = &sem2;
    sem_init(&sem2, 0, 0);
    char buf2[50000];
    targ2.shm = buf2;

    //set up the third buffer to be be 50000 long since the is the maximum allowed input
    targ3.sem = &sem3;
    sem_init(&sem3, 0, 0);
    char buf3[50000];
    targ3.shm = buf3;

    //check arguments for specified input and output files
    if( argc >= 3){
        if(strncmp(argv[1], "<", 1) == 0)
            inputFile(argv[2]);
        else if(strncmp(argv[1], ">", 1) == 0)
            outputFile(argv[2]);
        if(argc >= 5){
            if(strncmp(argv[3], "<", 1) == 0)
                inputFile(argv[4]);
            else if(strncmp(argv[3], ">", 1) == 0)
                outputFile(argv[4]);
        }
    }

    //create all four threads then join them when it is done running
    pthread_t Input, Newline, Plus, Format; 

    pthread_create(&Input, NULL, &getInput, NULL);
    pthread_create(&Newline, NULL, &replaceNewline, NULL);
    pthread_create(&Plus, NULL, &replaceplusplus, NULL);
    pthread_create(&Format, NULL, &formatLines, NULL);

    pthread_join(Input, NULL);
    pthread_join(Newline, NULL);
    pthread_join(Plus, NULL);
    pthread_join(Format, NULL);

}