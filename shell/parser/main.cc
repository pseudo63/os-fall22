#include "parser.h"


#include <sstream>
#include <iostream>
#include <string.h>
#include <vector>

//This is a tester for the parser library
int main()
{
    std::string command;
    std::cout << "> ";
    std::getline(std::cin, command);
	//split the command by whitespace
    std::vector <std::string> commands = parse_from_line(command);
	//print the tokens
    for (const std::string &token : commands) {
         std::cout << "t : " << token << std::endl;
    }
    return 0;
}