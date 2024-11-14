#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#define INPUTSIZE 50
#define ENVSIZE 40
#include <sys/wait.h>

typedef struct{
    char* var;
    char* val;
}env;

enum command{
    HISTORY,
    EXIT,
    CHPROMPT,
    CLEAR,
    CD,
    PWD,
    SET,
    UNSET,
    ECHO,
    HISCOM
};
 

int countingN(int n) {
    
    int count = 0;

    while (n >= 1) {
        n /= 10;

        ++count;
    }

    return count;

}

int myFgets(char* arr, int fd) {


    //waiting for new command

    arr = fgets(arr, INPUTSIZE, stdin);

    if (arr == NULL) {
        perror("Fgets in myFgets failed\n");

        exit(EXIT_FAILURE);
    }

    if (strcmp(arr, "history\n") && arr[0] != '!'){

    static int n = 1;//storing command number

    char nstr[5];

    sprintf(nstr, "%d", n);

    short byte = countingN(n);//Counting digit

    write(fd, nstr, byte);//putting command number

    write(fd, " ", 1);

    ++n;

     if (write(fd, arr, strlen(arr)) == -1) {
        perror("Write in myfgets failed");

        exit(EXIT_FAILURE);
     }

    }

     //Deleting newLINE

    while (*arr != '\0'){
        if (*arr == '\n' && *++arr == '\0') {
            *--arr = '\0';
        }

        ++arr;
    }

    

    return 0;

}

int inputWait(char* input, char* prompt, int fdHis) {

    printf("%s", prompt);

    return myFgets(input, fdHis);

}

int history(int fdHist) {
    int size = lseek(fdHist, 0, SEEK_CUR);

    char* buf = (char*)malloc(size * sizeof(char));

    if (buf == NULL) {
        perror("Allocation in history command failed\n");
        
        exit(EXIT_FAILURE);
    } 

    if (pread(fdHist, buf, size - 1, 0) == -1) {
        perror("Reading history file was failed please try again\n");
     
        return 2;
    }

    printf("%s\n", buf);

    free(buf);

    return 0;
}

int clear(char* input, int fdHist) {

    pid_t pid = fork();

    if (pid == 0) {
        execlp("clear", "clear", NULL);
    }else if (pid > 0) {
        int stat;

        wait(&stat);

        if (WIFSIGNALED(stat)) {
            printf("Clear Child was signaled by %d\n", WTERMSIG(stat));

            return 2;
        } 
    }else {
        perror("Clear Child fork failed please try again\n");

        return 2;
    }

    return 0;// inputWait(input, fdHist);
}


int CommandPars(char* input) {

    if (!(strcmp(input, "history"))) {
        return HISTORY;
    }else if(!(strcmp(input, "clear"))) {
        return CLEAR;
    }else if (!(strcmp(input, "exit"))) {
        return EXIT;
    }else if (!(strncmp(input, "chprompt ", 9))){
        return CHPROMPT;
    }else if (!(strncmp(input, "cd ", 3))){
        return CD;
    }else if (!(strcmp(input, "pwd"))) {
        return PWD;
    }else if (!(strncmp(input, "setenv ", 7))){
        return SET;
    }else if (!(strncmp(input, "echo ", 5))) {
        return ECHO;
    }else if (!(strncmp(input, "unsetenv ", 9))) {
        return UNSET;
    }else if (!(strncmp(input, "!", 1))){
        return HISCOM;
    }else {
        return -1;        
    }
    
}

void mysetenv(char* input, env* environments) {
    char* var = strtok(input, " ");
    
    char* val = strtok(NULL, "");

    for (int index = 0 ; index < ENVSIZE ; ++index) {

    if (var && environments[index].var == NULL) {
        environments[index].var = malloc(strlen(var) + 1);
        strcpy(environments[index].var, var);
    }

    if (val && environments[index].val == NULL) {
        environments[index].val = malloc(strlen(val) + 1);
        strcpy(environments[index].val, val);
        break;
    }

    }
}

void envinit(env* environments) {
    for (int i = 0 ; i < ENVSIZE ; ++i) {
        environments[i].var = NULL;
        environments[i].val = NULL;
    }
}

void echo(char* input, env* environments) {
    if (*(input + 5) == '$') {
        char* var = strtok(input + 6, "");
        for (int i = 0 ; i < ENVSIZE ; ++i){
            if (environments[i].var != NULL) {
                if (strcmp(environments[i].var, var) == 0) {
                    printf("%s\n", environments[i].val);
                    return;
                }
            }
        }
    }
    printf("%s\n", input + 5);
    
}

void myunsetenv(char* input, env* environments) {
        char* var = strtok(input + 9, "");
        for (int i = 0 ; environments[i].var != NULL && i < ENVSIZE ; ++i){
            if (strcmp(environments[i].var, var) == 0) {
                free(environments[i].var);
                free(environments[i].val);
                environments[i].var = NULL;
                environments[i].val = NULL;
                return;
            }
        }
}

int getnum(char* input) {

    int result = 0;

    while (*input != '\0') {
        if (*input < '0' || *input > '9') {return -1;}

        result *= 10;

        result += *input - 48;

        ++input;
    }
    return result;
}

int gethistorycom(int fdHist, int target, char* input) {
    int n = 1;

    lseek(fdHist, 0, SEEK_SET);

    while (read(fdHist, input, INPUTSIZE)) {
        int i = 0;
        for (i = 0 ; i < 50 && n != target; ++i) {
            if (input[i] == '\n') {
                ++n;
            }
        }

        if (n == target) {
            i += 2;
            lseek(fdHist, i, SEEK_SET);
            read(fdHist, input, INPUTSIZE);

            for (i = 0 ; input[i] != '\n' ; ++i) {}
            input[i] = '\0';

            return CommandPars(input);
        }
    }
    return -1;
}