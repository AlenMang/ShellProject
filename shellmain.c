#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#define INPUTSIZE 50
#include "shellfoos.c"
#define ENVSIZE 40




int main() {

    //Hi PLEASE READ GITHUB FOR APPLICATION DOCUMENTATION

    env* environments = malloc(ENVSIZE * sizeof(env));

    envinit(environments);

    char* pwdstore = malloc(INPUTSIZE * sizeof(char));

    strcpy(pwdstore, "/home/user");

    char* input = malloc(INPUTSIZE * sizeof(char));

    if (input == NULL) {
        perror("Allocation for input failed\n");

        exit(EXIT_FAILURE);
    }

    int fdHist = open("shellhistory.txt", O_RDWR | O_TRUNC | O_CREAT, 0666);

    if (fdHist == -1) {
        perror("Opening history file was failed\n");

        exit(EXIT_FAILURE);
    }

    int fdPrompt = open("promptinfo.txt", O_RDWR | O_TRUNC | O_CREAT, 0666);

    int i = 0;

    if (fdPrompt == -1) {
        perror("Opening prompt info file was failed\n");

        exit(EXIT_FAILURE);
    }

    if (write(fdPrompt, "myshell> ", 9) == -1) {
        perror("Writing in fdPrompt failed\n");

        exit(EXIT_FAILURE);
    }

    char* prompt = malloc(10 * sizeof(char));//9 = myshell str size

    pread(fdPrompt, prompt, 10, 0);

    do{
        if (inputWait(input, prompt, fdHist) == 2){continue;}//trying again, something went wrong


        int command = CommandPars(input);
    
        if (command == HISCOM) {

            int num = getnum(input + 1);
            
            if (num == -1) {
                command = -1;
            }else {
            command = gethistorycom(fdHist, num, input);
            printf("%s\n", input);
            }
        }
        
        switch(command) {
            case HISTORY: {
                if(history(fdHist) == 2) {
                    printf("History command was failed, please try again\n");
                };
                break;
            }
            case CLEAR: {
                if (clear(input, fdHist) == 2) {
                    printf("Clear command was failed, please try again\n");
                };
                break;
            }
            case EXIT: {
                free(input);

                close(fdHist);

                close(fdPrompt);

                free(prompt);

                free(environments);

                return 0;
            }
            case CHPROMPT: {

                int size = strlen(input + 9);//9 = chprompt 

                prompt = realloc(prompt, size + 1);

                if (prompt == NULL) {
                    perror("Realocating prompt was failed");
                    free(input);
                    free(prompt);
                    close(fdHist);
                    close(fdPrompt);
                    free(environments);
                    exit(EXIT_FAILURE);
                }
                

                for (int i = 0, j = 9 ; i < size + 1 ; ++i, ++j) {

                    prompt[i] = input[j];

                } 

                break;
            }case CD: {
                int size = strlen(input + 3);

                pwdstore = realloc(pwdstore, size + 1);

                for (int i = 0, j = 3 ; i < size + 1 ; ++i, ++j) {
                    pwdstore[i] = input[j];
                }

                break;


            }case PWD: {
                printf("%s\n", pwdstore);
                break;
            }case SET: {
                mysetenv(input + 7, environments);            
                break;
            }case ECHO: {
                echo(input, environments);
                break;
            }case UNSET: {
                printf("UNSETING..\n");
                myunsetenv(input, environments);
                break;
            }default:
                printf("command not found\n");  

        }


    }while (strcmp(input, "exit"));

    free(input);

    free(environments);

    free(prompt);

    close(fdHist);

    return 0;
    
}