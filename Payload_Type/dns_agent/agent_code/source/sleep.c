/**
 * sleep.c - Sleep interval command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/sleep.h"
 #include "../include/Config.h"
 
 /**
  * Change the sleep interval
  */
 char* cmd_sleep(const char* args) {
     if (args == NULL || *args == '\0') {
         return _strdup("Error: No sleep interval specified");
     }
     
     // Parse the sleep interval
     int interval = atoi(args);
     if (interval <= 0) {
         return _strdup("Error: Invalid sleep interval");
     }
     
     // Convert to milliseconds
     DWORD sleep_interval = interval * 1000;
     
     // Set the sleep interval
     set_sleep_interval(sleep_interval);
     
     // Return success message
     char* result = (char*)malloc(64);
     if (result != NULL) {
         sprintf_s(result, 64, "Sleep interval set to %d seconds", interval);
     }
     return result;
 }
 