#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/Config.h"

#define DEBUG_MODE 1
#define USE_HMAC

// Parámetros por defecto
static char*        c2_domain         = NULL;
static const char*  agent_id          = "eff1d21a-3fb3-434b-8db3-1f6f3510623b";
static DWORD        sleep_interval    = 15 * 1000;
static DWORD        jitter_percentage = 10;
static DWORD        kill_date         = 0;
// DNS específicos
static char*        msginit           = NULL;
static char*        msgdefault        = NULL;
static const char*  hmac_key          = "13a6868413d6f20d9ae58dcbb82f136b506c85964dab3c2f303764c6b1f89318";

void reset_config(void) {
#if DEBUG_MODE
    printf("[CFG] reset_config()\n");
#endif
    if (c2_domain) free(c2_domain);
    c2_domain = _strdup("c2.example.com");
#if DEBUG_MODE
    printf("[CFG] c2_domain = %s\n", c2_domain);
#endif

    sleep_interval    = 15 * 1000;
    jitter_percentage = 10;
#if DEBUG_MODE
    printf("[CFG] sleep_interval=%lu jitter_percentage=%lu\n",
           sleep_interval, jitter_percentage);
#endif

    kill_date = 0;
#if DEBUG_MODE
    printf("[CFG] kill_date = %u\n", kill_date);
#endif

    if (msginit) free(msginit);
    msginit    = _strdup("app");
#if DEBUG_MODE
    printf("[CFG] msginit = %s\n", msginit);
#endif

    if (msgdefault) free(msgdefault);
    msgdefault = _strdup("dash");
#if DEBUG_MODE
    printf("[CFG] msgdefault = %s\n", msgdefault);
#endif
}

const char* get_c2_domain(void)           { return c2_domain; }
void        set_c2_domain(const char* d)  { if (c2_domain) free(c2_domain); c2_domain = _strdup(d); }

const char* get_agent_id(void)            { return agent_id; }

DWORD       get_sleep_interval(void)      { return sleep_interval; }
void        set_sleep_interval(DWORD i)   { sleep_interval = i; }

DWORD       get_jitter_percentage(void)   { return jitter_percentage; }
void        set_jitter_percentage(DWORD j){ jitter_percentage = j; }

const char* get_msginit(void)             { return msginit; }
void        set_msginit(const char* p)    { if (msginit) free(msginit); msginit = _strdup(p); }

const char* get_msgdefault(void)          { return msgdefault; }
void        set_msgdefault(const char* t) { if (msgdefault) free(msgdefault); msgdefault = _strdup(t); }

DWORD       get_kill_date(void)           { return kill_date; }
BOOL        is_kill_date_passed(void)     { return FALSE; }

const char* get_hmac_key(void)            { return hmac_key; }
