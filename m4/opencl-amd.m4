AC_DEFUN([YOO_CHECK_OPENCL], [
    no_opencl=yes
    YOOCL_CFLAGS=
    YOOCL_LIBS=

    AS_IF(
        [test "x$AMDAPPSDKROOT" == "x"],
        [echo "checking for OpenCL... no"],
        [
            echo "checking for OpenCL... yes"
            no_opencl=no
            YOOCL_CFLAGS='-DHAS_OPENCL -I${AMDAPPSDKROOT}/include'
            YOOCL_LIBS='-lOpenCL -L${AMDAPPSDKROOT}/lib/x86_64'
        ]
    )
])

AC_DEFUN([YOO_ENABLE_OPENCL], [

    AC_ARG_ENABLE([opencl],
        AS_HELP_STRING([--enable-opencl], [enable OpenCL, default: auto]),
        [
            AS_CASE( [$enable_val],

                [yes],
                [
                    YOO_CHECK_OPENCL
                    AS_IF(
                        [test "x$no_opencl" == "xyes"],
                        [],
                        [AC_MSG_ERROR([OpenCL is not found.])])
                ],

                [no],
                [
                    no_opencl=yes
                    YOOCL_CFLAGS=
                    YOOCL_LIBS=
                ],

                [auto],
                [YOO_CHECK_OPENCL]

                [AC_MSG_ERROR([bad value ${enableval} for --enable-opencl])]
                
            )
        ],
    [
        YOO_CHECK_OPENCL
    ])
   
    AC_SUBST(YOOCL_CFLAGS)
    AC_SUBST(YOOCL_LIBS)
])
