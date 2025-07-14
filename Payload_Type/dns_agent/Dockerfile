FROM itsafeaturemythic/mythic_python_base:latest

USER root
WORKDIR /Mythic

# Solo si necesitas mingw por la compilación dentro del contenedor
RUN apt-get update && \
    apt-get install -y --no-install-recommends mingw-w64 build-essential && \
    rm -rf /var/lib/apt/lists/*

COPY main.py           ./
COPY build.py          ./
COPY agent_functions/  ./agent_functions/
COPY agent_code/       ./agent_code/

ENTRYPOINT ["python3", "main.py"]

