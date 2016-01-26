bin_PROGRAMS = yoo-fsm
noinst_LIBRARIES = libyoofsm.a

if ENABLE_DEBUG
EXTRA_CFLAGS = -g3 -O0 -Wall -Werror
else
EXTRA_CFLAGS = -Ofast
endif
EXTRA_CFLAGS += -I../include/ -D_GNU_SOURCE

libyoofsm_a_CFLAGS = $(EXTRA_CFLAGS) -I../include/ $(YOOSTDLIB_CFLAGS)
libyoofsm_a_SOURCES = common.c

yoo_fsm_CFLAGS = $(EXTRA_CFLAGS) $(YOOSTDLIB_CFLAGS)
yoo_fsm_LDADD = libyoofsm.a $(YOOSTDLIB_LIBS)
yoo_fsm_SOURCES = main.c
