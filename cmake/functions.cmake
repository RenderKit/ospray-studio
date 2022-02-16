## Copyright 2020-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

function(_target_strip_and_sign target)
    if(OSPSTUDIO_SIGN_FILE)
        set(OSPSTUDIO_SIGN_FILE_ARGS -q -vv)
        if(APPLE)
            list(APPEND OSPSTUDIO_SIGN_FILE_ARGS -o runtime -e ${CMAKE_SOURCE_DIR}/gitlab/ospray_studio.entitlements)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_STRIP} -x $<TARGET_FILE:${target}>
                COMMAND ${OSPSTUDIO_SIGN_FILE} ${OSPSTUDIO_SIGN_FILE_ARGS} $<TARGET_FILE:${target}>
                COMMENT "Stripping and signing ${target}"
                VERBATIM
            )
        elseif(WIN32)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${OSPSTUDIO_SIGN_FILE} ${OSPSTUDIO_SIGN_FILE_ARGS} $<TARGET_FILE:${target}>
                COMMENT "Signing ${target}"
                VERBATIM
            )
        elseif(UNIX)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${target}>
                # COMMAND ${OSPSTUDIO_SIGN_FILE} ${OSPSTUDIO_SIGN_FILE_ARGS} $<TARGET_FILE:${target}>
                COMMENT "Stripping ${target}"
                VERBATIM
            )
        endif()
    endif()
endfunction()
