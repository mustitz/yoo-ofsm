noinst_PROGRAMS = yoo-build-poker-ofsm



if ENABLE_DEBUG
EXTRA_CFLAGS = -g3 -O0 -Wall -Werror -DDEBUG_MODE
else
EXTRA_CFLAGS = -Ofast
endif
EXTRA_CFLAGS += -I../../include/ -D_GNU_SOURCE



yoo_build_poker_ofsm_CFLAGS = $(EXTRA_CFLAGS) $(YOOSTDLIB_CFLAGS) $(YOOCL_CFLAGS)
yoo_build_poker_ofsm_LDADD = -L./../../source/.libs/ -lyooofsmlib $(YOOSTDLIB_LIBS) $(YOOCL_LIBS)
yoo_build_poker_ofsm_SOURCES = main.c opencl.c poker.c deck-36.c deck-52.c
