#include "../include/global.h"
#include "../include/logger.h"
#include <string>

using namespace std;

void printSuccessForCommand(string userCommand) { //This will print a success message when a command works correctly
    cse4589_print_and_log("[%s:SUCCESS]\n", userCommand.c_str()); //This will display "[COMMAND:SUCCESS]"
}

void printCommandEndTag(string commandNameInput) { //This will add an END tag afterr the command is done printing
    cse4589_print_and_log("[%s:END]\n", commandNameInput.c_str()); //This will display [COMMAND:END]
}

void printErrorForCommand(string badCommandName) { //This will print an error if something fails
    cse4589_print_and_log("[%s:ERROR]\n",badCommandName.c_str()); //This will display [COMMAND:ERROR]
    printCommandEndTag(badCommandName); //This will also end the command with [COMMAND:END]
}