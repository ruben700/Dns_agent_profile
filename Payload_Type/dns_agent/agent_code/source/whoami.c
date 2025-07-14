/**
 * whoami.c - Print current user command for DNS C2 Agent
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/whoami.h"
#include "../include/Utils.h"

/**
 * cmd_whoami - devuelve "HOSTNAME\USERNAME" o un mensaje de error.
 */
char* cmd_whoami(const char* args) {
    (void)args;  // evitamos warning de parámetro sin usar

    // 1) Obtener usuario
    char* username = get_username();
    if (!username) {
        return _strdup("Error: Could not get username");
    }

    // 2) Obtener hostname (o dominio)
    char* hostname = get_hostname();
    if (!hostname) {
        free(username);
        return _strdup("Error: Could not get hostname");
    }

    // 3) Calcular tamaño: hostname + '\' + username + '\0'
    size_t host_len = strlen(hostname);
    size_t user_len = strlen(username);
    size_t needed   = host_len + 1 /* '\\' */ + user_len + 1 /* '\0' */;

    // 4) Reservar buffer
    char* result = (char*)malloc(needed);
    if (!result) {
        free(username);
        free(hostname);
        return _strdup("Error: memory allocation failed");
    }

    // 5) Formatear "HOSTNAME\USERNAME"
    sprintf_s(result, needed, "%s\\%s", hostname, username);

    // 6) Liberar temporales y devolver
    free(username);
    free(hostname);
    return result;
}
