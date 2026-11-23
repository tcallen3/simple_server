CFLAGS = -g3 -Wall -Werror -Wextra -Wformat=2 -Wjump-misses-init 
CFLAGS += -Wlogical-op -Wpedantic -Wshadow

# Overlay is needed to get some things like setprogname to work on Linux and 
# avoids any header namespace changes (alternative is to manually add "bsd/" 
# in front of numerous files). Strictly this should be split into CFLAGS and 
# LD_FLAGS portions as:
#
# CFLAGS += $(shell pkg-config --cflags libbsd-overlay) 
# LD_FLAGS += $(shell pkg-config --libs libbsd-overlay)
#
# but we're already being lazy and this is a small project with a simple
# structure.

CFLAGS += $(shell pkg-config --cflags libbsd-overlay) 
LD_FLAGS = $(shell pkg-config --libs libbsd-overlay)

PROG = sws

SRC = main.c network.c settings.c logging.c
BIN = bin

all: ${PROG}

depend:
	mkdep -- ${CFLAGS} *.c

${PROG}: ${SRC}
	mkdir -p ${BIN}
	${CC} ${CFLAGS} -o ${BIN}/${PROG} ${SRC} ${LD_FLAGS}

clean:
	rm -rf ${BIN}/${PROG}
