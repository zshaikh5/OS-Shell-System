// Zaina Shaih. 
// CSS 430 - Prog 1 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fcntl.h> 

using namespace std;
#define MAX_LINE 80

// osh> print
void printPrompt() {
    cout << "osh> ";
    cout.flush();
}

// Takes in user input 
string readInput() {
    string input;
    getline(cin, input);
    return input;
}

// Function to okenize the input string by splitting it into individual tokens
void tokenizeInput(const string &input, char *args[]) {
    int i = 0;
     // Tokenizing the input string
    char *token = strtok(const_cast<char*>(input.c_str()), " ");
    while (token != nullptr) {
        args[i++] = token;
        token = strtok(nullptr, " ");
    }
    args[i] = nullptr; 
}

// Function to handle input/output redirection
void handleRedirection(char *args[]) {
    int i = 0;
    int fd;
    while (args[i] != nullptr) {
        if (strcmp(args[i], ">") == 0) {
            args[i] = nullptr; 
            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (fd < 0) {
                perror("Failed to open file for writing");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO); 
            close(fd); 
            break;
        } else if (strcmp(args[i], "<") == 0) {
            args[i] = nullptr;
            fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("Failed to open file for reading");
                exit(1);
            }
            dup2(fd, STDIN_FILENO); 
            close(fd); 
            break;
        }
        i++;
    }
}

// Function to execute a command - forks new process to execute command
void executeCommand(char *args[], bool background_process) {
    pid_t rc = fork();
    if (rc < 0) {
        perror("fork() failed");
        exit(1);
    } else if (rc == 0) { // Child process
        handleRedirection(args);  
        execvp(args[0], args);   
        perror("execvp error"); 
        exit(1); 
    } else {
        if (!background_process) {
            waitpid(rc, NULL, 0); 
        } else {
            cout << "Background process running, PID: " << rc << endl;
        }
    }
}

// Function to handle a command with a pipe
void executePipeCommand(char *leftCmd[], char *rightCmd[], bool background_process) {
    int pipefd[2]; 
    if (pipe(pipefd) == -1) {
        perror("pipe() failed");
        exit(1);
    }

    pid_t rc1 = fork();
    if (rc1 < 0) {
        perror("fork() failed");
        exit(1);
    } else if (rc1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); 
        close(pipefd[1]); 

        execvp(leftCmd[0], leftCmd);
        perror("execvp error for left command");
        exit(1); 
    }

    pid_t rc2 = fork();
    if (rc2 < 0) {
        perror("fork() failed");
        exit(1);
    } else if (rc2 == 0) {
        dup2(pipefd[0], STDIN_FILENO); 
        close(pipefd[1]); 
        close(pipefd[0]); 

        execvp(rightCmd[0], rightCmd);
        perror("execvp error for right command");
        exit(1); 
    }

    close(pipefd[0]);
    close(pipefd[1]);

    if (!background_process) {
        waitpid(rc1, NULL, 0);
        waitpid(rc2, NULL, 0);
    } else {
        cout << "Background processes running, PIDs: " << rc1 << " and " << rc2 << endl;
    }
}

// Function to checks if input command contains a pipe ("|"). If found, splits into left and right, otherwise false
bool containsPipe(char *args[], char *leftCmd[], char *rightCmd[]) {
    int i = 0, j = 0, pipePos = -1;

    while (args[i] != nullptr) {
        if (strcmp(args[i], "|") == 0) {
            pipePos = i;
            break;
        }
        i++;
    }

    if (pipePos == -1) {
        return false; 
    }

    for (i = 0; i < pipePos; i++) {
        leftCmd[i] = args[i];
    }
    leftCmd[i] = nullptr;

    for (i = pipePos + 1; args[i] != nullptr; i++) {
        rightCmd[j++] = args[i];
    }
    rightCmd[j] = nullptr;

    return true; 
}

// Main function to run program by reading user input and processing commands
int main() {
    char *args[MAX_LINE / 2 + 1];
    char *leftCmd[MAX_LINE / 2 + 1]; 
    char *rightCmd[MAX_LINE / 2 + 1]; 
    int should_run = 1; 
    vector<string> history; 

    while (should_run) {
        printPrompt(); 
        string input = readInput();

        if (input == "!!") {
            if (!history.empty()) {
                input = history.back(); 
                cout << input << endl; 
            } else {
                cout << "No commands in history." << endl;
                continue;
            }
        }

        if (!input.empty()) {
            history.push_back(input); 
        }

        tokenizeInput(input, args); 

        if (args[0] != nullptr && strcmp(args[0], "exit") == 0) {
            return 0; 
        }

        int i = 0;
        while (args[i] != nullptr) i++; 

        bool back_proc = (i > 0 && strcmp(args[i - 1], "&") == 0);
        if (back_proc) {
            args[i - 1] = nullptr; 
        }

        if (containsPipe(args, leftCmd, rightCmd)) {
            executePipeCommand(leftCmd, rightCmd, back_proc);
        } else {
            executeCommand(args, back_proc);
        }
    }
    return 0; 
}