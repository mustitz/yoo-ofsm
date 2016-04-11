LOG_DRIVER = ./validation.sh

TESTS = run-outsider

check_PROGRAMS = outsider

if ENABLE_DEBUG
EXTRA_CFLAGS = -g3 -O0 -Wall -Werror
else
EXTRA_CFLAGS = -Ofast
endif
EXTRA_CFLAGS += -I${srcdir}/../include -D_GNU_SOURCE

outsider_CFLAGS = $(EXTRA_CFLAGS) $(YOOSTDLIB_CFLAGS)
outsider_LDADD = $(YOOSTDLIB_LIBS) -L./../source/.libs/ -lyooofsmlib
outsider_SOURCES = outsider.c

.PHONY : run-outsider

EXTRA_DIST = validation.sh
