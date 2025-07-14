/**
 * Config.h - Configuración para el DNS C2 Agent
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

// Restablecer todo a valores por defecto
void reset_config(void);

// --------------------
// Parámetros básicos
// --------------------

// Dominio C2 (o IP)
const char* get_c2_domain(void);
void        set_c2_domain(const char* domain);

// ID del agente (UUID de payload)
const char* get_agent_id(void);


// Intervalo de "sleep" (milisegundos)
DWORD       get_sleep_interval(void);
void        set_sleep_interval(DWORD interval);

// Porcentaje de "jitter"
DWORD       get_jitter_percentage(void);
void        set_jitter_percentage(DWORD percentage);

// HMAC shared secret (si estás compilando con USE_HMAC)

const char* get_hmac_key(void);


// --------------------
// Parámetros DNS extra
// --------------------

// Prefijo de subdominio inicial (msginit)
const char* get_msginit(void);
void        set_msginit(const char* prefix);

// Tipo de mensaje por defecto (msgdefault)
const char* get_msgdefault(void);
void        set_msgdefault(const char* type);

// --------------------
// Kill date
// --------------------

DWORD       get_kill_date(void);
BOOL        is_kill_date_passed(void);

// Aliases por compatibilidad
#define get_jitter_percent   get_jitter_percentage
#define set_jitter_percent   set_jitter_percentage

#endif // CONFIG_H
