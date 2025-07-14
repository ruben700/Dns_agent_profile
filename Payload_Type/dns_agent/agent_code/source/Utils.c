/**
 * Utils.c - Utility functions for DNS C2 Agent
 * 
 * Contains base64 encoding/decoding, string manipulation,
 * and other helper functions needed by the agent.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <objbase.h>
#include <rpc.h>
#include "../include/Utils.h"

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* base64_encode(const unsigned char* data, size_t input_length, size_t* output_length) {
    size_t encoded_length = 4 * ((input_length + 2) / 3);
    char* encoded_data = (char*)malloc(encoded_length + 1);
    if (!encoded_data) return NULL;

    size_t i = 0, j = 0;
    while (i < input_length) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        uint32_t triple  = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded_data[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded_data[j++] = base64_table[(triple >>  6) & 0x3F];
        encoded_data[j++] = base64_table[ triple        & 0x3F];
    }

    switch (input_length % 3) {
        case 1: encoded_data[encoded_length - 2] = '=';
                encoded_data[encoded_length - 1] = '=';
                break;
        case 2: encoded_data[encoded_length - 1] = '=';
                break;
    }
    encoded_data[encoded_length] = '\0';
    if (output_length) *output_length = encoded_length;
    return encoded_data;
}

unsigned char* base64_decode(const char* data, size_t input_length, size_t* output_length) {
    // Remove whitespace
    char* clean_data = malloc(input_length + 1);
    if (!clean_data) return NULL;
    size_t ci = 0;
    for (size_t k = 0; k < input_length; k++) {
        if (!isspace((unsigned char)data[k])) clean_data[ci++] = data[k];
    }
    clean_data[ci] = '\0';

    if (ci % 4 != 0) { free(clean_data); return NULL; }
    size_t padding = (ci >= 2 && clean_data[ci - 1] == '=') + (ci >= 2 && clean_data[ci - 2] == '=');
    size_t decoded_length = (ci / 4) * 3 - padding;
    unsigned char* decoded = malloc(decoded_length + 1);
    if (!decoded) { free(clean_data); return NULL; }

    uint32_t sextet[4];
    size_t di = 0;
    for (size_t i = 0; i < ci; i += 4) {
        for (int k = 0; k < 4; k++) {
            char c = clean_data[i + k];
            if (c == '=') sextet[k] = 0;
            else if (c >= 'A' && c <= 'Z') sextet[k] = c - 'A';
            else if (c >= 'a' && c <= 'z') sextet[k] = c - 'a' + 26;
            else if (c >= '0' && c <= '9') sextet[k] = c - '0' + 52;
            else if (c == '+') sextet[k] = 62;
            else if (c == '/') sextet[k] = 63;
            else { free(clean_data); free(decoded); return NULL; }
        }
        uint32_t triple = (sextet[0] << 18) | (sextet[1] << 12) | (sextet[2] << 6) | sextet[3];
        if (di < decoded_length) decoded[di++] = (triple >> 16) & 0xFF;
        if (di < decoded_length) decoded[di++] = (triple >>  8) & 0xFF;
        if (di < decoded_length) decoded[di++] =  triple        & 0xFF;
    }
    decoded[decoded_length] = '\0';
    if (output_length) *output_length = decoded_length;
    free(clean_data);
    return decoded;
}

char* generate_uuid() {
    UUID uuid;
    if (UuidCreate(&uuid) != RPC_S_OK) return NULL;
    char* str = malloc(37);
    if (!str) return NULL;
    snprintf(str, 37, "%08lX-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
             uuid.Data1, uuid.Data2, uuid.Data3,
             (uuid.Data4[0] << 8) | uuid.Data4[1],
             uuid.Data4[2], uuid.Data4[3], uuid.Data4[4],
             uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]);
    return str;
}

char* get_timestamp() {
    char* ts = malloc(20);
    if (!ts) return NULL;
    SYSTEMTIME st; GetLocalTime(&st);
    sprintf_s(ts, 20, "%04d-%02d-%02d %02d:%02d:%02d",
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond);
    return ts;
}

char** str_split(const char* str, char delim, int* count) {
    if (!str || !count) return NULL;
    int n = 1;
    for (const char* p = str; *p; ++p) if (*p == delim) n++;
    char** toks = malloc(n * sizeof(char*));
    if (!toks) { *count = 0; return NULL; }
    const char* start = str; int idx = 0;
    for (const char* p = str; ; ++p) {
        if (*p == delim || *p == '\0') {
            size_t L = p - start;
            toks[idx] = malloc(L + 1);
            if (toks[idx]) { memcpy(toks[idx], start, L); toks[idx][L] = '\0'; }
            idx++; start = p + 1;
        }
        if (*p == '\0') break;
    }
    *count = n; return toks;
}

void free_tokens(char** tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; i++) free(tokens[i]);
    free(tokens);
}

char* url_encode(const char* str) {
    if (!str) return NULL;
    size_t L = strlen(str), cap = L*3 + 1;
    char* enc = malloc(cap);
    if (!enc) return NULL;
    char* p = enc;
    for (; *str; str++) {
        unsigned char c = (unsigned char)*str;
        if (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~') *p++ = c;
        else { sprintf_s(p, 4, "%%%02X", c); p+=3; }
    }
    *p = '\0';
    return enc;
}

BOOL file_exists(const char* f) {
    DWORD a = GetFileAttributesA(f);
    return a!=INVALID_FILE_ATTRIBUTES && !(a&FILE_ATTRIBUTE_DIRECTORY);
}
BOOL directory_exists(const char* d) {
    DWORD a = GetFileAttributesA(d);
    return a!=INVALID_FILE_ATTRIBUTES && (a&FILE_ATTRIBUTE_DIRECTORY);
}
BOOL create_directory(const char* d) {
    return CreateDirectoryA(d, NULL);
}
char* get_hostname() {
    DWORD sz = MAX_COMPUTERNAME_LENGTH+1;
    char* h = malloc(sz);
    if (h && GetComputerNameA(h, &sz)) return h;
    free(h); return NULL;
}
char* get_username() {
    DWORD sz = 256; char* u = malloc(sz);
    if (u && GetUserNameA(u, &sz)) return u;
    free(u); return NULL;
}
char* get_current_directory() {
    DWORD sz = GetCurrentDirectoryA(0, NULL);
    char* d = malloc(sz);
    if (d && GetCurrentDirectoryA(sz, d)) return d;
    free(d); return NULL;
}
BOOL copy_file(const char* s, const char* d, BOOL ow) {
    return CopyFileA(s, d, !ow);
}
BOOL move_file(const char* s, const char* d) {
    return MoveFileA(s, d);
}
BOOL delete_file(const char* f) {
    return DeleteFileA(f);
}
long get_file_size(const char* f) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(f, GetFileExInfoStandard, &fad)) return -1;
    LARGE_INTEGER li; li.HighPart = fad.nFileSizeHigh; li.LowPart = fad.nFileSizeLow;
    return (long)li.QuadPart;
}
unsigned char* read_file(const char* f, size_t* sz) {
    FILE* F; if (fopen_s(&F, f, "rb")!=0) return NULL;
    fseek(F, 0, SEEK_END); *sz = ftell(F); rewind(F);
    unsigned char* buf = malloc(*sz+1);
    if (!buf || fread(buf,1,*sz,F)!=*sz) { free(buf); fclose(F); return NULL; }
    buf[*sz]='\0'; fclose(F); return buf;
}
BOOL write_file(const char* f, const unsigned char* d, size_t sz) {
    FILE*F; if (fopen_s(&F,f,"wb")!=0) return FALSE;
    size_t w = fwrite(d,1,sz,F); fclose(F); return w==sz;
}
BOOL append_to_file(const char* f, const unsigned char* d, size_t sz) {
    FILE*F; if (fopen_s(&F,f,"ab")!=0) return FALSE;
    size_t w = fwrite(d,1,sz,F); fclose(F); return w==sz;
}
void sleep_ms(DWORD ms) { Sleep(ms); }

int get_random_int(int min, int max) {
    static int seeded = 0;
    if (max <= min) return min;
    if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }
    return min + (rand() % (max - min + 1));
}

/**
 * Apply jitter to a sleep interval.
 * Returns the interval +/- jitter_percent%
 * E.g.: base=60000, jitter=10 → in [54000 .. 66000]
 */
DWORD apply_jitter(DWORD base_ms, DWORD jitter_percent) {
    if (jitter_percent == 0 || base_ms == 0) return base_ms;
    DWORD max_j = (base_ms * jitter_percent) / 100;
    int offset = get_random_int(-(int)max_j, (int)max_j);
    return base_ms + offset;
}

void str_tolower(char* s) {
    for (; *s; s++) *s = tolower((unsigned char)*s);
}
void str_toupper(char* s) {
    for (; *s; s++) *s = toupper((unsigned char)*s);
}

BOOL str_startswith(const char* s, const char* pref) {
    size_t lp = strlen(pref), ls = strlen(s);
    return (ls >= lp && strncmp(s, pref, lp) == 0);
}
BOOL str_endswith(const char* s, const char* suf) {
    size_t ls = strlen(s), lf = strlen(suf);
    return (ls >= lf && strcmp(s + ls - lf, suf) == 0);
}

char* str_trim(const char* str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') {
        char* e = malloc(1); if (e) *e = '\0'; return e;
    }
    const char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    size_t len = (end - str) + 1;
    char* out = malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, str, len);
    out[len] = '\0';
    return out;
}
