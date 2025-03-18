#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

void generateRandNum(int *min, int *max, int *num) {
    *num = *min + rand() % *max; // generates a random number between min and max
}

void childAnswer(int *childRead, int *childNum, int *childWrite){
    if (*childRead > *childNum) { *childWrite = -1;} // when the guess is too high
    else if (*childRead < *childNum){ *childWrite = 1;} // when the guess is too low
    else {*childWrite = 0;} // when the guess is correct
}

void parentGuess(int *min, int *max, int *parentNum, int *parentRead){
    if (*parentRead == -1) { *max = *parentNum - 1;} // adjusts max limit
    else if (*parentRead == 1) { *min = *parentNum + 1;} // adjusts min limit
    *parentNum = (*min + *max ) / 2; // guessing a new number
}

int main(int argc, char *argv[]){
    srand(time(NULL)); // Seed the random number generator
    int parentNum = 0, childNum = 0, parentRead = -2, childRead = 0, childWrite = 0, min = 1, max = 999, counter = 0;
    int childReadParentWrite[2], childWriteParentChild[2]; // first and second pipe, 0 - read, 1 - write 

    if (pipe(childReadParentWrite) == -1 || pipe(childWriteParentChild) == -1) {
        printf("Pipe Failed");
        return 1;
    }

    int pId = fork(); // forking the process into parent and child

    if (pId < 0) {
        printf("ERROR!");
        return 2;
    }

    else if (pId == 0) {  // child process 
        close(childReadParentWrite[1]); // Close writing end of first pipe
        close(childWriteParentChild[0]); // Close reading end of second pipe
        generateRandNum(&min, &max, &childNum); // generate a random number for the child
       
        while (childRead != childNum) {
            // Reading a number from parent using first pipe
            if (read(childReadParentWrite[0], &childRead, sizeof(childRead)) < 0) {
                printf("ERROR in childReadParentWrite!");
                return 3;
            }
            
            childAnswer(&childRead, &childNum, &childWrite); // compare parent's number and child's number
            
            if (write(childWriteParentChild[1], &childWrite, sizeof(childWrite)) < 0 ) { // write the result
                printf("ERROR in childWriteParentChild!");
                return 4;
            }
        }
        close(childReadParentWrite[0]); // Close reading end of first pipe
        close(childWriteParentChild[1]); // Close writing end of second pipe
    }

    else { // Parent process
        close(childReadParentWrite[0]); // Close reading end of first pipe
        close(childWriteParentChild[1]); // Close writing end of second pipe        
        parentNum = (min + max) / 2; // initial guess by parent
        counter = 0; // counting attempts

        while (parentRead != 0) {
            if (write(childReadParentWrite[1], &parentNum, sizeof(parentNum)) < 0) {
                printf("Error in childReadParentWrite!");

                if (kill(pId, SIGTERM) == -1) {
                    printf("Error sending termination signal to child process");
                    printf("Attempting to forcefully terminate the child process...\n");

                    if (kill(pId, SIGKILL) == -1) {
                        printf("Error forcefully terminating child process");
                    }
                }
                return 5;
            } 

            // Read number from child
            if (read(childWriteParentChild[0], &parentRead, sizeof(parentRead)) < 0) {
                printf("Error in childWriteParentChild!");

                if (kill(pId, SIGTERM) == -1) {
                    printf("Error sending termination signal to child process");
                    printf("Attempting to forcefully terminate the child process...\n");

                    if (kill(pId, SIGKILL) == -1) {
                        printf("Error forcefully terminating child process");
                    }
                }
                return 6;
            }
        
            parentGuess(&min, &max, &parentNum, &parentRead); // adjust the guess based on child's response
            counter++; // counting attempts
        }
        close(childReadParentWrite[1]); // Close writing end of first pipe
        close(childWriteParentChild[0]); // Close reading end of second pipe
        printf("Number = %d\nParent process used: %d attempts.\n", parentNum, counter);

        int status;
        if (wait(&status) == -1) {
            printf("Error waiting for child process");
            return 7;
        }

        // Check if child didn't terminate
        if (!WIFEXITED(status) && !WIFSIGNALED(status)) {
            printf("Child process did not terminate normally");

            if (kill(pId, SIGTERM) == -1) {
                printf("Error sending termination signal to child process");
                printf("Attempting to forcefully terminate the child process...\n");

                if (kill(pId, SIGKILL) == -1) {
                    printf("Error forcefully terminating child process");
                }
            }
        }
    }
    return 0;
}