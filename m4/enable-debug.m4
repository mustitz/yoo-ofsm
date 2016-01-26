AC_DEFUN([YOO_ENABLE_DEBUG], [

    AC_ARG_ENABLE([debug],
        AS_HELP_STRING([--enable-debug], [enable debugging, default: no]),
        [case "${enableval}" in
            yes) debug=true ;;
            no)  debug=false ;;
            *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
        esac],
    [debug=false])

    AM_CONDITIONAL(ENABLE_DEBUG, test x"$debug" = x"true")

])
