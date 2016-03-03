bin_PROGRAMS = yoo-ofsm
noinst_LIBRARIES = libyoofsm.a

if ENABLE_DEBUG
EXTRA_CFLAGS = -g3 -O0 -Wall -Werror
else
EXTRA_CFLAGS = -Ofast
endif
EXTRA_CFLAGS += -I../include/ -D_GNU_SOURCE

libyoofsm_a_CFLAGS = $(EXTRA_CFLAGS) -I../include/ $(YOOSTDLIB_CFLAGS)
libyoofsm_a_SOURCES = common.c

yoo_ofsm_CFLAGS = $(EXTRA_CFLAGS) $(YOOSTDLIB_CFLAGS)
yoo_ofsm_LDADD = libyoofsm.a $(YOOSTDLIB_LIBS)
yoo_ofsm_SOURCES = main.c deck-36.c deck-52.c
