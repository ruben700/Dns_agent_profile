# Makefile (en agent_code/)

# Usamos el cross-compiler de mingw
CC       := x86_64-w64-mingw32-gcc
INCDIR   := include

# Si quieres una build optimizada, llama a `make OPT=1`
ifeq ($(OPT),1)
CFLAGS   := -Wall -Wextra -O2 -I$(INCDIR)
else
# Modo debug por defecto: símbolos de depuración, sin optimizar, y activa DEBUG_MODE
CFLAGS   := -Wall -Wextra -g -O0 -DDEBUG_MODE=1 -I$(INCDIR)
endif

LDFLAGS  := -lws2_32 -liphlpapi -ldnsapi -lshlwapi -lole32 -lrpcrt4 -luuid -lbcrypt

SRCDIR   := source
OBJDIR   := obj
BINDIR   := bin

# Todos los .c en source/, más main.c
SRCS     := $(wildcard $(SRCDIR)/*.c) main.c
# Para cada source/foo.c → obj/source/foo.o
OBJS     := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/source/%.o,$(wildcard $(SRCDIR)/*.c)) \
            $(OBJDIR)/main.o

TARGET   := $(BINDIR)/mythic_dns_agent.exe

.PHONY: all dirs clean

# Target por defecto
all: dirs $(TARGET)

# Crear todos los dirs que faltan
dirs:
	@mkdir -p $(OBJDIR)/source
	@mkdir -p $(BINDIR)

# Linkeo final: SOLO enlazamos los .o en $(OBJS)
$(TARGET): $(OBJS)
	@echo "[LD] $@"
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Compilar cada fuente en source/
$(OBJDIR)/source/%.o: $(SRCDIR)/%.c | dirs
	@echo "[CC] $< → $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar main.c
$(OBJDIR)/main.o: main.c | dirs
	@echo "[CC] $< → $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar
clean:
	@echo "[CLEAN]"
	rm -rf $(OBJDIR) $(BINDIR)
