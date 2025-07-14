#!/usr/bin/env python3
import sys
import os
from pathlib import Path

# 1) Asegúrate de que el directorio actual (donde están build.py y agent_functions/)
#    esté en el PYTHONPATH para que se encuentren tus módulos.
here = Path(__file__).parent.resolve()
if str(here) not in sys.path:
    sys.path.insert(0, str(here))

# 2) Importa tu PayloadType para que el service lo registre
import build               # <-- tu build.py estilo B

# 3) Importa tus comandos para que Mythic los encuentre
import agent_functions     # <-- dentro tienes cd.py, ls.py, etc.

# 4) Arranca el servicio
from mythic_container.mythic_service import start_and_run_forever
if __name__ == "__main__":
    start_and_run_forever()
