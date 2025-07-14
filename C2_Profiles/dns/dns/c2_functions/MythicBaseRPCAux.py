from aio_pika import connect_robust, IncomingMessage, Message
import asyncio
import uuid
import json
import logging
import os
from enum import Enum

RABBITMQ_HOST = os.getenv("RABBITMQ_HOST", "mythic_rabbitmq")
RABBITMQ_USER = os.getenv("RABBITMQ_USER", "mythic_user")
RABBITMQ_PASS = os.getenv("RABBITMQ_PASSWORD", "")
RABBITMQ_VHOST = os.getenv("RABBITMQ_VHOST", "mythic_vhost")
RABBITMQ_PORT = int(os.getenv("RABBITMQ_PORT", 5672))

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


class MythicStatusAux(Enum):
    Success = "success"
    Error = "error"


class RPCResponseAux:
    def __init__(self, resp: dict):
        self._raw_resp = resp
        if resp.get("success") is True:
            self.status = MythicStatusAux.Success
            self.response = resp.get("response", "")
            self.error_message = None
        else:
            self.status = MythicStatusAux.Error
            self.error_message = resp.get("error", "Unknown error")
            self.response = None

    @property
    def status(self):
        return self._status

    @status.setter
    def status(self, status):
        self._status = status

    @property
    def error_message(self):
        return self._error_message

    @error_message.setter
    def error_message(self, error_message):
        self._error_message = error_message

    @property
    def response(self):
        return self._response

    @response.setter
    def response(self, response):
        self._response = response


class MythicBaseRPCAux:
    def __init__(self):
        self.connection = None
        self.channel = None
        self.callback_queue = None
        self.futures = {}
        self.loop = asyncio.get_event_loop()

    async def connect(self):
        try:
            amqp_url = f"amqp://{RABBITMQ_USER}:{RABBITMQ_PASS}@{RABBITMQ_HOST}:{RABBITMQ_PORT}/{RABBITMQ_VHOST}"
            logging.info(f"[RPC] Connecting to {amqp_url}")

            self.connection = await connect_robust(
                host=RABBITMQ_HOST,
                port=RABBITMQ_PORT,
                login=RABBITMQ_USER,
                password=RABBITMQ_PASS,
                virtualhost=RABBITMQ_VHOST,
            )
            logging.debug(f"DEBUG: user={RABBITMQ_USER} pass={RABBITMQ_PASS} host={RABBITMQ_HOST} port={RABBITMQ_PORT}")
            self.channel = await self.connection.channel()
            self.callback_queue = await self.channel.declare_queue(exclusive=True)
            await self.callback_queue.consume(self.on_response)
            logging.info("[RPC] Connected successfully to RabbitMQ")
            return self
        except Exception as e:
            logging.error(f"[RPC] Failed to connect to RabbitMQ: {str(e)}")
            raise

    def on_response(self, message: IncomingMessage):
        try:
            logging.info(f"[RPC] Received response: {message.body.decode()}")
            future = self.futures.pop(message.correlation_id)
            future.set_result(message.body)
            logging.info(f"[RPC] Response for correlation_id: {message.correlation_id} set successfully")
        except Exception as e:
            logging.error(f"[RPC] Error handling RPC response: {str(e)}")

    async def send_mythic_rpc_message(self, message_body: dict, routing_key: str, timeout: int = 10) -> RPCResponseAux:
        if self.connection is None:
            await self.connect()
        correlation_id = str(uuid.uuid4())
        future = self.loop.create_future()
        self.futures[correlation_id] = future
    
        try:
            await self.channel.default_exchange.publish(
                Message(
                    json.dumps(message_body).encode(),
                    content_type="application/json",
                    correlation_id=correlation_id,
                    reply_to=self.callback_queue.name,
                ),
                routing_key=routing_key,
            )
            logging.info(f"[RPC] Sent RPC call to {routing_key} with correlation_id: {correlation_id}")
            # Esperar con timeout seguro
            result = await asyncio.wait_for(future, timeout=timeout)
            return RPCResponseAux(json.loads(result))
        except asyncio.TimeoutError:
            logging.error(f"[RPC] Timeout after {timeout} seconds waiting for response from {routing_key}")
            raise TimeoutError(f"RPC Timeout: No response from {routing_key} within {timeout} seconds")
        except Exception as e:
            logging.error(f"[RPC] RPC call failed: {e}", exc_info=True)
            raise


    async def call(self, message_body: dict, receiver: str = "mythic_rpc_callback_create") -> RPCResponseAux:
        return await self.send_mythic_rpc_message(message_body, routing_key=receiver)
