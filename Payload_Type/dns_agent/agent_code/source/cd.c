/**
 * cd.c - Change directory command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <direct.h>
 #include "../include/cd.h"
 #include "../include/Utils.h"
 #include "../include/pwd.h"
 
 /**
  * Change the current directory
  */
 char* cmd_cd(const char* args) {
     if (args == NULL || *args == '\0') {
         return _strdup("Error: No directory specified");
     }
     
     // Convert to wide characters
     int wlen = MultiByteToWideChar(CP_UTF8, 0, args, -1, NULL, 0);
     wchar_t* wdir = (wchar_t*)malloc(wlen * sizeof(wchar_t));
     if (wdir == NULL) {
         return _strdup("Error: Memory allocation failed");
     }
     MultiByteToWideChar(CP_UTF8, 0, args, -1, wdir, wlen);
     
     // Use Unicode version of chdir
     if (_wchdir(wdir) != 0) {
         char* result = (char*)malloc(256);
         if (result != NULL) {
             sprintf_s(result, 256, "Error: Could not change to directory '%s'", args);
         }
         free(wdir);
         return result;
     }
     
     free(wdir);
     return cmd_pwd(NULL);  // Return the new current directory
 }
 