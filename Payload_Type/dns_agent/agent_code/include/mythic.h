#ifndef MYTHIC_H
#define MYTHIC_H

#include <windows.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tipos de mensaje que intercambia el agente
typedef enum {
    MYTHIC_MESSAGE_TYPE_CHECKIN  = 1,
    MYTHIC_MESSAGE_TYPE_TASKING  = 2,
    MYTHIC_MESSAGE_TYPE_RESPONSE = 3
} MythicMessageType;

// Datos del sistema que mandamos en el check-in
typedef struct {
    char hostname[256];
    char username[256];
    char ip[16];
    char os[256];
    char os_version[256];
    char architecture[16];
    char pid[16];
    char uuid[37];         // UUID del agente
} MythicCheckin;

// Representa un tasking recibido de Mythic
typedef struct {
    char id[37];           // UUID de la tarea (NUL-terminated)
    char command[64];      // Nombre de comando
    char params[4096];     // JSON de parámetros
} MythicTask;

/**
 * @brief Inicializa la integración Mythic (transporte, UUID, logging…).
 * @return TRUE si todo OK; FALSE en error.
 */
BOOL mythic_init();

/**
 * @brief Libera recursos inicializados en mythic_init().
 */
void mythic_cleanup();

/**
 * @brief Crea un payload listo para enviar como check-in.
 * @param checkin      Puntero a MythicCheckin con datos del sistema
 * @param message_len  Retorna longitud del buffer creado.
 * @return Buffer alocado (caller debe free()); NULL en error.
 */
char* mythic_create_checkin_message(const MythicCheckin* checkin, size_t* message_len);

/**
 * @brief Parsea un payload recibido en JSON a MythicTask.
 * @param message      Buffer crudo recibido.
 * @param message_len  Longitud de message en bytes.
 * @param task         Puntero a MythicTask donde guardar el resultado.
 * @return TRUE si parseó correctamente; FALSE si hubo error o payload inválido.
 */
BOOL mythic_parse_tasking_message(const char* message, size_t message_len, MythicTask* task);

/**
 * @brief Crea un payload de respuesta para un tasking.
 * @param task_id      UUID de la tarea que estamos respondiendo.
 * @param output       Cadena de salida/resultados.
 * @param message_len  Retorna longitud del buffer creado.
 * @return Buffer alocado (caller debe free()); NULL en error.
 */
char* mythic_create_response_message(const char* task_id, const char* output, size_t* message_len);

/**
 * @brief Obtiene el UUID único asignado al agente.
 * @return Puntero a cadena interna (no free).
 */
const char* mythic_get_uuid();

/**
 * @brief Inyecta o actualiza el UUID (p.ej. en pruebas).
 * @param uuid  Cadena NUL-terminated de 36+1 chars
 */
void mythic_set_uuid(const char* uuid);

/**
 * @brief Registra el JSON recibido en el check-in inicial como callback en Mythic.
 * @param checkin_json  JSON completo devuelto por transport_perform_checkin().
 */
void register_callback(const char* checkin_json);

#ifdef __cplusplus
}
#endif

#endif // MYTHIC_H
