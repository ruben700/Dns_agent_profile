FROM itsafeaturemythic/mythic_python:latest



# Instalar librerías Python necesarias
RUN pip install --no-cache-dir dnslib requests

# Establecer directorio de trabajo
WORKDIR /Mythic/

# Copiar el código fuente completo al contenedor
COPY ./ /Mythic/

# Declarar variable de entorno base
ENV PYTHONUNBUFFERED=1

# Comando de inicio directo (sin env_injector.py)
CMD ["python3", "main.py"]
