AC_INIT([yoo-ofsm], [0.1], [mustitz@gmail.com])
${CFLAGS=""}

AM_INIT_AUTOMAKE(dist-bzip2)
AC_CONFIG_MACRO_DIR([m4])

PKG_CHECK_MODULES([YOOSTDLIB], [yoostdlib >= 0.1])

YOO_ENABLE_DEBUG
YOO_WITH_VALGRIND

AC_PROG_CC_C99
AC_PROG_RANLIB
AM_SILENT_RULES([yes])

AC_CONFIG_FILES([
    Makefile 
    source/Makefile 
    include/Makefile
    validation/Makefile
    validation/validation.sh
],[
    chmod +x validation/validation.sh
])

AC_OUTPUT
