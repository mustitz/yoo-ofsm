AC_DEFUN([YOO_WITH_VALGRIND], [
    valgrind_withval=auto

    AC_ARG_WITH([valgrind],
      AS_HELP_STRING([--with-valrgind], [Use valgrind to run testsuite]),
      [valgrind_withval=$withval]
    )

    AS_CASE(
      [$valgrind_withval],

      [no],
      [
        valgrind_withval=
      ],

      [yes],
      [
        AC_CHECK_PROG([have_valgrind], [valgrind], [yes], [no])
        AS_IF(
          [test "x$have_valgrind" == "xyes"],
          [valgrind_withval=valgrind],
          [AC_MSG_ERROR([Valgrind not found in PATH.])])
      ],
      
      [auto],
      [
        AC_CHECK_PROG([have_valgrind], [valgrind], [yes], [no])
        AS_IF(
          [test "x$have_valgrind" == "xyes"], 
          [valgrind_withval=valgrind],
          [valgrind_withval=])
      ],

      [
        AC_CHECK_FILE([$valgrind_withval], [have_valgrind=yes], [have_valgrind=no])
        AS_IF(
          [test "x$have_valgrind" != "xyes"],
          [
            valgrind_withval=
            AC_MSG_ERROR([Valgrind not found in PATH.])
          ])
      ]
    )

    AC_SUBST([VALGRIND], [$valgrind_withval])
])
