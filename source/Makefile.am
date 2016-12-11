noinst_PROGRAMS = yoo-build-poker-ofsm yoo-verify-poker-ofsm
noinst_LIBRARIES = libyoodeck.a

if ENABLE_DEBUG
EXTRA_CFLAGS = -g3 -O0 -Wall -Werror
else
EXTRA_CFLAGS = -Ofast
endif
EXTRA_CFLAGS += -I../include/ -D_GNU_SOURCE



lib_LTLIBRARIES = libyooofsmlib.la 
libyooofsmlib_la_SOURCES = common.c
libyooofsmlib_la_CFLAGS = $(EXTRA_CFLAGS) -I../include/ $(YOOSTDLIB_CFLAGS)
libyooofsmlib_la_LIBADD = $(YOOSTDLIB_LIBS)



libyoodeck_a_CFLAGS = $(EXTRA_CFLAGS) -I../include/ $(YOOSTDLIB_CFLAGS) $(YOOCL_CFLAGS)
libyoodeck_a_SOURCES = utils.c deck-36.c deck-52.c



yoo_build_poker_ofsm_CFLAGS = $(EXTRA_CFLAGS) $(YOOSTDLIB_CFLAGS) $(YOOCL_CFLAGS)
yoo_build_poker_ofsm_LDADD = libyoodeck.a libyooofsmlib.la $(YOOSTDLIB_LIBS) $(YOOCL_LIBS)
yoo_build_poker_ofsm_SOURCES = build.c opencl.c

yoo_verify_poker_ofsm_CFLAGS = $(EXTRA_CFLAGS) $(YOOSTDLIB_CFLAGS)
yoo_verify_poker_ofsm_LDADD = libyoodeck.a libyooofsmlib.la $(YOOSTDLIB_LIBS) $(YOOCL_LIBS)
yoo_verify_poker_ofsm_SOURCES = verify.c opencl.c
