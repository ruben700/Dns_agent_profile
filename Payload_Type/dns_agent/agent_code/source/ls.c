/**
 * ls.c - List directory command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/ls.h"
 #include "../include/Utils.h"
 
 // Maximum output size
 #define MAX_OUTPUT_SIZE 8192
 
 /**
  * List the contents of a directory
  */
 char* cmd_ls(const char* args) {
    const char* target = (args && *args) ? args : ".";
    char* dir = _strdup(target);
     if (dir == NULL) {
         return _strdup("Error: Could not get directory");
     }
     
     // Convert directory path to wide characters
     int wlen = MultiByteToWideChar(CP_UTF8, 0, dir, -1, NULL, 0);
     wchar_t* wdir = (wchar_t*)malloc(wlen * sizeof(wchar_t));
     if (wdir == NULL) {
         free(dir);
         return _strdup("Error: Memory allocation failed");
     }
     MultiByteToWideChar(CP_UTF8, 0, dir, -1, wdir, wlen);
     
     // Create search path
     size_t plen = wcslen(wdir) + 3;  // espacio para "\*" y '\0'
     wchar_t* wsearch_path = malloc(plen * sizeof(wchar_t));
     if (!wsearch_path) {
         free(dir); free(wdir);
         return _strdup("Error: Memory allocation failed");
     }
     wcscpy_s(wsearch_path, plen, wdir);
     wcscat_s(wsearch_path, plen, L"\\*");;
     
     WIN32_FIND_DATAW find_data;
     HANDLE find_handle = FindFirstFileW(wsearch_path, &find_data);
     free(wsearch_path);
     
     if (find_handle == INVALID_HANDLE_VALUE) {
         char* result = (char*)malloc(256);
         if (result != NULL) {
             sprintf_s(result, 256, "Error: Could not list directory '%s'", dir);
         }
         free(dir);
         free(wdir);
         return result;
     }
     
     // Allocate buffer for results
     char* result = (char*)malloc(MAX_OUTPUT_SIZE);
     if (result == NULL) {
         FindClose(find_handle);
         free(dir);
         free(wdir);
         return _strdup("Error: Memory allocation failed");
     }
     
     // Initialize result buffer
     sprintf_s(result, MAX_OUTPUT_SIZE, "Directory listing for %s:\n\n", dir);
     size_t result_len = strlen(result);
     
     // Process directory entries
     do {
         // Convert filename from wide char to UTF-8
         char filename[MAX_PATH];
         WideCharToMultiByte(CP_UTF8, 0, find_data.cFileName, -1, filename, MAX_PATH, NULL, NULL);
         
         // Skip . and .. entries
         if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
             continue;
         }
         
         // Format the file/directory information
         char entry[MAX_PATH + 100];
         SYSTEMTIME st;
         FileTimeToSystemTime(&find_data.ftLastWriteTime, &st);
         
         sprintf_s(entry, sizeof(entry), "%02d/%02d/%04d  %02d:%02d    %s%s%s\n",
                  st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute,
                  (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>  " : "       ",
                  (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "" : "  ",
                  filename);
         
         // Check if buffer is large enough
         if (result_len + strlen(entry) >= MAX_OUTPUT_SIZE - 1) {
             strcat_s(result, MAX_OUTPUT_SIZE, "\n... (output truncated)");
             break;
         }
         
         // Append entry to result
         strcat_s(result, MAX_OUTPUT_SIZE, entry);
         result_len += strlen(entry);
         
     } while (FindNextFileW(find_handle, &find_data));
     
     FindClose(find_handle);
     free(dir);
     free(wdir);
     
     return result;
 }
 