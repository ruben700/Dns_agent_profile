import json
import logging
import base64
from dns.c2_functions.MythicBaseRPCAux import MythicBaseRPCAux, RPCResponseAux
from aio_pika import Message

# ——— Configuración básica de tu logging ———
#   - filename: fichero de salida
#   - level: DEBUG para que el filtro vea TODO
logging.basicConfig(
    filename="/tmp/mythic_rpc.log",
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(message)s",
)
logger = logging.getLogger()

# ─── Silenciar logs “de sistema” que no te interesan ───
for lib in (
    "pamqp",
    "pamqp.transport",
    "pika",
    "aio_pika",
    "aiormq",
):
    logging.getLogger(lib).setLevel(logging.WARNING)

# ─── Filtrar / mapear niveles y módulos ───
class OnlyMyLogsFilter(logging.Filter):
    EXCLUDE_PREFIXES = (
        "pamqp",
        "pika",
        "aio_pika",
        "aiormq",
    )
    ALLOWED_LEVELS = (logging.DEBUG, logging.INFO, logging.CRITICAL)

    def filter(self, record):
        # 1) Excluir TODO lo que venga de estos módulos
        for p in self.EXCLUDE_PREFIXES:
            if record.name.startswith(p):
                return False
        # 2) Solo dejar pasar DEBUG, INFO, CRITICAL
        if record.levelno in self.ALLOWED_LEVELS:
            # Mapear DEBUG/CRITICAL a INFO para que en el fichero veas sólo [INFO]
            record.levelno = logging.INFO
            record.levelname = "INFO"
            return True
        return False

# 3) Añadimos el filtro a todos los handlers del root logger
for handler in logger.handlers:
    handler.addFilter(OnlyMyLogsFilter())

# 4) Seguimos con nivel DEBUG para que el filtro reciba TODO
logger.setLevel(logging.DEBUG)

class MythicRPCResponseAux(RPCResponseAux):
    def __init__(self, resp: RPCResponseAux):
        super().__init__(resp._raw_resp)
        raw = resp._raw_resp
        if isinstance(raw, (bytes, bytearray)):
            raw = raw.decode()
        if isinstance(raw, str):
            try:
                parsed = json.loads(raw)
            except Exception:
                parsed = {}
                print(f"[!] Failed to JSON-decode raw RPC response: {raw}")
        elif isinstance(raw, dict):
            parsed = raw
        else:
            parsed = {}
        # si success == True guardamos parsed, si no data=None
        if parsed.get("success"):
            self.data = parsed
        else:
            self.data = None
        # guardamos también el parsed completo para debug
        self.raw = parsed

    @property
    def callback_id(self):
        return self.data.get("callback_id") if self.data else None

    @property
    def callback_uuid(self):
        return self.data.get("callback_uuid") if self.data else None


class MythicCallbackRPCAux(MythicBaseRPCAux):
    async def create_callback(
        self,
        ip: str,
        user: str,
        host: str,
        pid: int,
        os: str,
        architecture: str,
        description: str,
        extra_info: str,
        sleep_info: str,
        payload_uuid: str,
        c2_profile: str = "dns"
    ) -> MythicRPCResponseAux:
        print(f"[RPC] create_callback called with payload_uuid={payload_uuid}")
        resp = await self.call(
            {
                "action":         "create_callback",
                "c2_profile":     c2_profile,
                "ip":             ip,
                "user":           user,
                "host":           host,
                "pid":            pid,
                "os":             os,
                "architecture":   architecture,
                "description":    description,
                "extra_info":     extra_info,
                "sleep_info":     sleep_info,
                "payload_uuid":   payload_uuid,
            },
            receiver="mythic_rpc_callback_create"
        )
        print(f"[RPC] create_callback raw response: {resp._raw_resp}")
        aux = MythicRPCResponseAux(resp)
        print(f"[RPC] create_callback parsed data: {aux.data}")
        return aux

    async def checkin_get_tasking(
        self,
        callback_uuid: str,
        size: int = 1,
        payload_type: str = "dns-agent"  
    ) -> MythicRPCResponseAux:
        body = {
            "agent_callback_id": callback_uuid,
            "c2_profile":        "dns",
            "payload_type":      payload_type,
            "message": {
                "action":       "get_tasking",
                "tasking_size": size
            }
        }
        resp = await self.call(body, receiver="mythic_rpc_handle_agent_json")
        aux = MythicRPCResponseAux(resp)
        print(f"[RPC] checkin_get_tasking → {aux.data}")
        return aux

    async def agent_post_response(self, responses: list) -> dict:
        """
        Envía por RabbitMQ un mensaje 'agent_message' con action='post_response'
        y el array de respuestas (cada una: task_id, base64(user_output), completed).
        Esto dispara la función Go util_agent_message_actions_post_response.go.
        """
        body = {
            "action": "post_response",
            "responses": responses
        }
        # rpc_execute es el método interno que publica al canal RabbitMQ
        # para los mensajes de tipo "agent_message"
        resp = await self.rpc_execute("agent_message", body)
        return resp

    async def handle_agent_message_json(
        self,
        *,
        agent_callback_id: str = None,
        callback_id: str = None,
        c2_profile: str,
        payload_type: str,
        message: dict
    ) -> MythicRPCResponseAux:
        """
        Envia un mensaje JSON al RPC MYTHIC_RPC_HANDLE_AGENT_JSON para
        procesar cualquier mensaje de agente (p.ej. post_response).
        """
        body = {
            "c2_profile":   c2_profile,
            "payload_type": payload_type,
            "message":      message,
        }
        if agent_callback_id:
            body["agent_callback_id"] = agent_callback_id
        elif callback_id:
            body["callback_id"] = callback_id
        else:
            raise ValueError("Se requiere callback_id o agent_callback_id")

        resp = await self.call(body, receiver="mythic_rpc_handle_agent_json")
        return MythicRPCResponseAux(resp)

    async def task_search(self, callback_id: int) -> MythicRPCResponseAux:
        print(f"[RPC] task_search called for callback_id={callback_id}")
        resp = await self.call(
            {
                "action":       "task_search",
                "callback_id":  callback_id
            },
            receiver="mythic_rpc_task_search"
        )
        print(f"[RPC] task_search raw response: {resp._raw_resp}")
        aux = MythicRPCResponseAux(resp)
        print(f"[RPC] task_search parsed data: {aux.data}")
        return aux


    async def _map_task_id(self, task_id):
        """
        Si me pasas un UUID (str), llamo a task_search() para
        localizar el entero que me exige response_create/search.
        """
        if isinstance(task_id, str):
            if not self._last_callback_display_id:
                raise RuntimeError("No tengo callback_display_id para mapear task UUID")
            ts = await self.task_search(self._last_callback_display_id)
            if not ts.data:
                raise RuntimeError(f"Error en task_search: {ts.raw.get('error')}")
            for t in ts.data.get("tasks", []):
                # suponiendo que el RPC task_search devuelve en cada `t`:
                #   - t["agent_task_id"] = UUID string
                #   - t["id"]            = DB id (int)
                if t.get("agent_task_id") == task_id:
                    return t["id"]
            raise KeyError(f"No encontré task con UUID {task_id}")
        return task_id
    
    
    async def send_post_response(
        self,
        agent_callback_id: str,
        task_uuid: str,
        output: bytes,
    ) -> MythicRPCResponseAux:
        """
        Envía la respuesta del agente (post_response) usando mythic_rpc_handle_agent_json.
        agent_callback_id: UUID interno del callback
        task_uuid:       UUID (string) de la task que devolvió get_tasking
        output:          stdout en bytes
        """
        payload = {
            "agent_callback_id": agent_callback_id,
            "c2_profile":        "dns",
            "payload_type":      "dns-agent",
            "message": {
                "action":   "post_response",
                "task_id":  task_uuid,
                "response": base64.b64encode(output).decode(),
            }
        }
        resp = await self.call(payload, receiver="mythic_rpc_handle_agent_json")
        return MythicRPCResponseAux(resp)
    
    async def add_route(
        self,
        source_uuid: str,
        destination_uuid: str,
        direction: int = 1,
        metadata: str = None
    ) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "add_route",
            "source": source_uuid,
            "destination": destination_uuid,
            "direction": direction,
            "metadata": metadata,
        })
        return MythicRPCResponseAux(resp)
    
    async def response_create(
        self,
        task_id: int,
        response: bytes
    ) -> MythicRPCResponseAux:
        """
        Llama a MYTHIC_RPC_RESPONSE_CREATE para subir bytes de respuesta de un task.
        """
        payload = {
            "task_id": task_id,
            "response": base64.b64encode(response).decode(),
        }
        resp = await self.call(payload, receiver="mythic_rpc_response_create")
        return MythicRPCResponseAux(resp)

    async def response_search(
        self,
        task_id: int
    ) -> MythicRPCResponseAux:
        """
        Llama a MYTHIC_RPC_RESPONSE_SEARCH para recuperar todas las respuestas de un task.
        """
        payload = {"task_id": task_id}
        resp = await self.call(payload, receiver="mythic_rpc_response_search")
        return MythicRPCResponseAux(resp)

    
    async def remove_route(
        self,
        source_uuid: str,
        destination_uuid: str,
        direction: int = 1,
        metadata: str = None
    ) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "remove_route",
            "source": source_uuid,
            "destination": destination_uuid,
            "direction": direction,
            "metadata": metadata,
        })
        return MythicRPCResponseAux(resp)

    async def get_callback_info(self, uuid: str) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "get_callback_info",
            "uuid": uuid,
        })
        return MythicRPCResponseAux(resp)

    async def get_encryption_data(
        self,
        uuid: str,
        profile: str
    ) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "get_encryption_data",
            "uuid": uuid,
            "c2_profile": profile,
        })
        return MythicRPCResponseAux(resp)

    async def update_callback_info(
        self,
        uuid: str,
        info: dict
    ) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "update_callback_info",
            "uuid": uuid,
            "data": info,
        })
        return MythicRPCResponseAux(resp)

    async def add_event_message(
        self,
        message: str,
        level: str = "info"
    ) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "add_event_message",
            "level": level,
            "message": message,
        })
        return MythicRPCResponseAux(resp)

    async def encrypt_bytes(
        self,
        data: bytes,
        uuid: str,
        with_uuid: bool = False
    ) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "encrypt_bytes",
            "data": base64.b64encode(data).decode(),
            "uuid": uuid,
            "with_uuid": with_uuid,
        })
        return MythicRPCResponseAux(resp)

    async def decrypt_bytes(
        self,
        data: str,
        uuid: str,
        with_uuid: bool = False
    ) -> MythicRPCResponseAux:
        resp = await self.call({
            "action": "decrypt_bytes",
            "data": data,
            "uuid": uuid,
            "with_uuid": with_uuid,
        })
        return MythicRPCResponseAux(resp)
