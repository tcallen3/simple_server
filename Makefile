TEST_CFLAGS = -g3 -Wall -Werror -Wextra -Wformat=2 -Wjump-misses-init -Wlogical-op -Wshadow

# Overlay is needed to get some things like setprogname to work on Linux and 
# avoids any header namespace changes (alternative is to manually add "bsd/" 
# in front of numerous files).
TEST_CFLAGS += $(shell pkg-config --cflags libbsd-overlay) 
LD_FLAGS = $(shell pkg-config --libs libbsd-overlay) -lmagic

CFLAGS = ${TEST_CFLAGS} -Wpedantic

PROG = sws
TEST = test

SRC = main.c network.c settings.c logging.c parser.c response.c
TEST_SRC = test.c network.c settings.c logging.c parser.c response.c
BIN = bin

all: ${PROG} ${TEST}

depend:
	mkdep -- ${CFLAGS} *.c

${PROG}: ${SRC}
	mkdir -p ${BIN}
	${CC} ${CFLAGS} -o ${BIN}/${PROG} ${SRC} ${LD_FLAGS}

${TEST}: ${TEST_SRC}
	mkdir -p ${BIN}
	${CC} ${TEST_CFLAGS} -o ${BIN}/${TEST} ${TEST_SRC} ${LD_FLAGS}

clean:
	rm -rf ${BIN}/${PROG}
