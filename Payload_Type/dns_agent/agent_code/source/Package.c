/**
 * Package.c - Data packaging for DNS C2 Agent
 * 
 * Handles packaging and unpackaging of data for transmission
 * between the agent and the C2 server.
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "../include/Package.h"
 #include "../include/Utils.h"
 
 /**
  * Create a data package
  * 
  * @param type The package type
  * @param data The data to package
  * @param data_len The length of the data
  * @param package_len Pointer to store the length of the package
  * @return The package (must be freed by caller)
  */
 unsigned char* create_package(PackageType type, const unsigned char* data, size_t data_len, size_t* package_len) {
     // Calculate the package size
     // Format: [TYPE(1)][LENGTH(4)][DATA(n)]
     *package_len = 1 + 4 + data_len;
     
     // Allocate memory for the package
     unsigned char* package = (unsigned char*)malloc(*package_len);
     if (package == NULL) {
         return NULL;
     }
     
     // Set the package type
     package[0] = (unsigned char)type;
     
     // Set the data length (little-endian)
     package[1] = (unsigned char)(data_len & 0xFF);
     package[2] = (unsigned char)((data_len >> 8) & 0xFF);
     package[3] = (unsigned char)((data_len >> 16) & 0xFF);
     package[4] = (unsigned char)((data_len >> 24) & 0xFF);
     
     // Copy the data
     memcpy(package + 5, data, data_len);
     
     return package;
 }
 
 /**
  * Parse a data package
  * 
  * @param package The package to parse
  * @param package_len The length of the package
  * @param type Pointer to store the package type
  * @param data Pointer to store the data
  * @param data_len Pointer to store the length of the data
  * @return TRUE if successful, FALSE otherwise
  */
 BOOL parse_package(const unsigned char* package, size_t package_len, PackageType* type, unsigned char** data, size_t* data_len) {
     // Check if the package is valid
     if (package == NULL || package_len < 5) {
         return FALSE;
     }
     
     // Get the package type
     *type = (PackageType)package[0];
     
     // Get the data length (little-endian)
     *data_len = package[1] | (package[2] << 8) | (package[3] << 16) | (package[4] << 24);
     
     // Check if the data length is valid
     if (*data_len > package_len - 5) {
         return FALSE;
     }
     
     // Allocate memory for the data
     *data = (unsigned char*)malloc(*data_len);
     if (*data == NULL) {
         return FALSE;
     }
     
     // Copy the data
     memcpy(*data, package + 5, *data_len);
     
     return TRUE;
 }
 
 /**
  * Create a command package
  * 
  * @param command The command
  * @param package_len Pointer to store the length of the package
  * @return The package (must be freed by caller)
  */
 unsigned char* create_command_package(const char* command, size_t* package_len) {
     return create_package(PACKAGE_TYPE_COMMAND, (const unsigned char*)command, strlen(command), package_len);
 }
 
 /**
  * Create a result package
  * 
  * @param result The result
  * @param package_len Pointer to store the length of the package
  * @return The package (must be freed by caller)
  */
 unsigned char* create_result_package(const char* result, size_t* package_len) {
     return create_package(PACKAGE_TYPE_RESULT, (const unsigned char*)result, strlen(result), package_len);
 }
 
 /**
  * Create a file package
  * 
  * @param file_data The file data
  * @param file_len The length of the file data
  * @param package_len Pointer to store the length of the package
  * @return The package (must be freed by caller)
  */
 unsigned char* create_file_package(const unsigned char* file_data, size_t file_len, size_t* package_len) {
     return create_package(PACKAGE_TYPE_FILE, file_data, file_len, package_len);
 }
 
 /**
  * Create a system info package
  * 
  * @param system_info The system information
  * @param package_len Pointer to store the length of the package
  * @return The package (must be freed by caller)
  */
 unsigned char* create_sysinfo_package(const char* system_info, size_t* package_len) {
     return create_package(PACKAGE_TYPE_SYSINFO, (const unsigned char*)system_info, strlen(system_info), package_len);
 }
 
 /**
  * Create an error package
  * 
  * @param error The error message
  * @param package_len Pointer to store the length of the package
  * @return The package (must be freed by caller)
  */
 unsigned char* create_error_package(const char* error, size_t* package_len) {
     return create_package(PACKAGE_TYPE_ERROR, (const unsigned char*)error, strlen(error), package_len);
 }
 