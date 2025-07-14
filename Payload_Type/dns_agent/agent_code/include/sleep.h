/**
 * sleep.h - Sleep interval command for DNS C2 Agent
 */

 #ifndef SLEEP_H
 #define SLEEP_H
 
 #include <windows.h>
 
 /**
  * Change the sleep interval
  * 
  * @param args The new sleep interval in seconds
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_sleep(const char* args);
 
 #endif // SLEEP_H
 