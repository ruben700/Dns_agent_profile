// Command.h
#include <windows.h>
#include <string.h>
#include <stdlib.h> 



#ifndef COMMAND_H
#define COMMAND_H

// Command function type
typedef char* (*CommandFunction)(const char* args);

// Initialize the command module
BOOL command_init();

// Clean up the command module
void command_cleanup();

// Execute a command
BOOL execute_command(const char* command_str, char** output);

// Command loop
int command_loop();
#endif

