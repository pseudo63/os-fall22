#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "parser.h"

typedef struct command {
    std::string inFile = "";
    std::string outFile = "";
    std::vector<std::string> cmd;
} command_t;

bool is_operator(std::string arg) {
    return (arg == "<" || arg == ">" || arg == "|");
}

void parse_and_run_command(const std::string &command) {
    std::vector<std::string> tokens = parse_from_line(command);
    if (tokens.size() == 0) {
        return;
    }
    
    if (tokens[0] == "exit") {
        exit(0);
    } else if (is_operator(tokens[0])) {
        std::cerr << "Invalid command: Expected word, got " << tokens[0] << std::endl;
        return;
    }

    std::vector<command_t*> commands;
    command_t *currentCommand = new command_t();
    currentCommand->cmd.push_back(tokens[0]);
    commands.push_back(currentCommand);

    for (unsigned int i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "<") {
            if (currentCommand->inFile != "") {
                std::cerr << "Invalid command: Only supports one input redirection" << std::endl;
                goto cleanup;
            } else if (tokens.size() <= i + 1) {
                std::cerr << "Invalid command: Ran out of input for redirection" << std::endl;
                goto cleanup;
            } else if (is_operator(tokens[i + 1])) {
                std::cerr << "Invalid command: Expected word for redirection, got " << tokens[i + 1] << std::endl;
                goto cleanup;
            } else {
                currentCommand->inFile = tokens[i + 1];
                i++;
            }
        } else if (tokens[i] == ">") {
            if (currentCommand->outFile != "") {
                std::cerr << "Invalid command: Only supports one output redirection" << std::endl;
                goto cleanup;
            } else if (tokens.size() <= i + 1) {
                std::cerr << "Invalid command: Ran out of input for redirection" << std::endl;
                goto cleanup;
            } else if (is_operator(tokens[i + 1])) {
                std::cerr << "Invalid command: Expected word for redirection, got " << tokens[i + 1] << std::endl;
                goto cleanup;
            } else {
                currentCommand->outFile = tokens[i + 1];
                i++;
            }
        } else if (tokens[i] == "|") {
            if (tokens.size() <= i + 1) {
                std::cerr << "Invalid command: Ran out of input after pipe" << std::endl;
                goto cleanup;
            } else if (is_operator(tokens[i + 1])) {
                std::cerr << "Invalid command: Expected word to start command, got " << tokens[i + 1] << std::endl;      
                goto cleanup;
            } else {
                currentCommand = new command_t();
                commands.push_back(currentCommand);
            }
        } else {
            currentCommand->cmd.push_back(tokens[i]);
        }
    }

    {
    
    
    std::vector<int> p_ids;

    // pipefd0 read end
    // pipefd1 write end

    //foo > test.txt | bar should result in foo’s standard output 
    // being the file test.txt and bar’s input being a pipe that 
    // immediately indicates end-of-file


    std::vector<int *> pipes;
    for (unsigned long i = 0; i < commands.size() - 1; i++) {
        pipes.push_back(new int[2]);
        if(pipe2(pipes[i], O_CLOEXEC) < 0) {
            perror("Creating pipe failed");
            for (int* p : pipes) {
                close(p[0]);
                close(p[1]);
                delete[] p;
            }
            goto cleanup;
        }
    }

    for (unsigned long i = 0; i < commands.size(); i++) {
        int p_id = fork();
        if (p_id > 0) {
            p_ids.push_back(p_id);

        } else if (p_id == 0) {
            // Code executed by child (exec)
            // Execv that takes array must be terminated by nullptr
            char** cstrTokens = new char*[commands[i]->cmd.size() + 1];

            for (unsigned long tokenIndex = 0; tokenIndex < commands[i]->cmd.size(); tokenIndex++) {
                cstrTokens[tokenIndex] = const_cast<char *>(commands[i]->cmd[tokenIndex].c_str());
            }
            cstrTokens[commands[i]->cmd.size()] = nullptr;

            // Pipes
            if (i != 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("Dup failed");
                    exit(255);
                }
                close(pipes[i - 1][0]);
            }
            if (i < pipes.size()) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("Dup failed");
                    exit(255);
                }
                close(pipes[i][1]);
            }

            // Redirection
            if (commands[i]->inFile != "") {
                close(STDIN_FILENO);
                int fd = open(commands[i]->inFile.c_str(), O_RDONLY);
                if (fd < 0) {
                    perror(cstrTokens[0]);
                    delete[] cstrTokens;
                    exit(255);
                }
            }
            if (commands[i]->outFile != "") {
                close(STDOUT_FILENO);
                int fd = open(commands[i]->outFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 00644);
                if (fd < 0) {
                    perror(cstrTokens[0]);
                    delete[] cstrTokens;
                    exit(255);
                } 
            }
            
            execv(cstrTokens[0], cstrTokens);
            
            perror(cstrTokens[0]);
            delete[] cstrTokens; 
            for (command_t* c : commands) {
                delete c;
            }
            exit(255);

        } else {
            perror("Fork failed");
        }
    }

    for (int* p : pipes) {
        close(p[0]);
        close(p[1]);
        delete[] p;
    }

    // Child always exits; only reachable by parent
    for (unsigned long i = 0; i < p_ids.size(); i++) {
        int wstatus;
        waitpid(p_ids[i], &wstatus, 0);
        if(WIFEXITED(wstatus)) {
            std::cout << commands[i]->cmd[0] << " exit status: " << WEXITSTATUS(wstatus) << std::endl;
        } else if (WIFSIGNALED(wstatus)){
            std::cerr << commands[i]->cmd[0] << " was terminated by signal " << strsignal(WTERMSIG(wstatus)) << std::endl;  
        } else {
            std::cerr << commands[i]->cmd[0] << " failed to exit, unknown reason.\n";
        }
    }

    }

    // This is good practice we swear
    // https://github.com/torvalds/linux/blob/master/Documentation/process/coding-style.rst#7-centralized-exiting-of-functions
    cleanup:
        for (command_t* c : commands) {
            delete c;
        }
        return;

}

int main(void) {
    std::string command;
    std::cout << "> ";
    while (std::getline(std::cin, command)) {
        parse_and_run_command(command);
        std::cout << "> ";
    }
    return 0;
}
