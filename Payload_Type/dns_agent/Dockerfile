FROM itsafeaturemythic/mythic_python_base:latest

USER root
WORKDIR /Mythic

# Solo si necesitas mingw por la compilación dentro del contenedor
RUN sed -i 's|http://deb.debian.org/debian|http://archive.debian.org/debian|g' /etc/apt/sources.list && \
    sed -i 's|http://security.debian.org/debian-security|http://archive.debian.org/debian-security|g' /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends mingw-w64 build-essential && \
    rm -rf /var/lib/apt/lists/*


COPY main.py           ./
COPY build.py          ./
COPY agent_functions/  ./agent_functions/
COPY agent_code/       ./agent_code/

ENTRYPOINT ["python3", "main.py"]

