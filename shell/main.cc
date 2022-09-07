#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "parser.h"

void parse_and_run_command(const std::string &command) {
    /* Note that this is not the correct way to test for the exit command.
       For example the command "   exit  " should also exit your shell.
     */
    std::vector<std::string> tokens = parse_from_line(command);
    if (tokens.size() > 0) {
        if (tokens[0] == "exit") {
            exit(0);
        }

        int wstatus;
        int p_id = fork();
        if (p_id > 0) {
            waitpid(p_id, &wstatus, 0);
            if(WIFEXITED(wstatus)) {
                std::cout << tokens[0] << " exit status: " << WEXITSTATUS(wstatus) << std::endl;
            } else if (WIFSIGNALED(wstatus)){
                std::cerr << tokens[0] << " was terminated by signal " << strsignal(WTERMSIG(wstatus)) << std::endl;  
            } else {
                std::cerr << tokens[0] << " failed to exit, unknown reason.\n";
            }
        } else if (p_id == 0) {
            // Code executed by child (exec)
            // Execv that takes array must be terminated by nullptr
            char** cstrTokens = new char*[tokens.size() + 1];

            for (unsigned long tokenIndex = 0; tokenIndex < tokens.size(); tokenIndex++) {
                cstrTokens[tokenIndex] = const_cast<char *>(tokens[tokenIndex].c_str());
            }
            cstrTokens[tokens.size()] = nullptr;
            
            execv(cstrTokens[0], cstrTokens);
            
            perror(cstrTokens[0]);
            delete[] cstrTokens; 
            exit(255);

        } else {
            perror("Fork failed");
        }
    }
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
