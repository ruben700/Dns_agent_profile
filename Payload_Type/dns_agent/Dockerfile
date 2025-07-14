FROM itsafeaturemythic/mythic_python_base:latest

USER root
WORKDIR /Mythic



COPY main.py           ./
COPY build.py          ./
COPY agent_functions/  ./agent_functions/
COPY agent_code/       ./agent_code/

ENTRYPOINT ["python3", "main.py"]

