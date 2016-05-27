if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Detect Linux distribution (if possible)
    execute_process(COMMAND "/usr/bin/lsb_release" "-is"
                    TIMEOUT 4
                    OUTPUT_VARIABLE LINUX_DISTRO_ID
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "/usr/bin/lsb_release" "-rs"
                    TIMEOUT 4
                    OUTPUT_VARIABLE LINUX_DISTRO_RELEASE
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Linux distro is: ${LINUX_DISTRO_ID} ${LINUX_DISTRO_RELEASE}")
endif()
