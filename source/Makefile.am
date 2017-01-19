lib_LTLIBRARIES = libyooofsmlib.la 



if ENABLE_DEBUG
EXTRA_CFLAGS = -g3 -O0 -Wall -Werror -DDEBUG_MODE
else
EXTRA_CFLAGS = -Ofast
endif
EXTRA_CFLAGS += -I../include/ -D_GNU_SOURCE



libyooofsmlib_la_SOURCES = common.c
libyooofsmlib_la_CFLAGS = $(EXTRA_CFLAGS) -I../include/ $(YOOSTDLIB_CFLAGS)
libyooofsmlib_la_LIBADD = $(YOOSTDLIB_LIBS)
