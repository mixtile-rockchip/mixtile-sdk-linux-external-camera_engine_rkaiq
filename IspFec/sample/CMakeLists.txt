if (${CMAKE_SYSTEM_NAME} EQUAL "")
    set(OS "linux")
else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} OS)
endif()

find_package(libdrm QUIET)

if (libdrm_FOUND)
    add_compile_options(${LIBDRM_CFLAGS})
    add_compile_options(-DISPDEMO_ENABLE_DRM=1)
else()
    # use the local drm library in demo dir
    if (TOOLCHAIN_SYSROOT STREQUAL "libc")
        if (ARCH STREQUAL "aarch64")
            set(LOCAL_DRM_LIB_DIR ${CMAKE_SOURCE_DIR}/sample/deps/lib/aarch64/)
        elseif (ARCH STREQUAL "arm")
            set(LOCAL_DRM_LIB_DIR ${CMAKE_SOURCE_DIR}/sample/deps/lib/arm/)
        endif()

        SET(LOCAL_DRM_LIB drm)

        message("\n****** build sample_fec with the local drm library: ${LOCAL_DRM_LIB_DIR}\n")
    endif()
endif()

set(ISPFEC_INSTALLED_DIR ${CMAKE_SOURCE_DIR}/build/${OS}/output/${ARCH}/installed/)
set(ISPFEC_LIB_DIR ${ISPFEC_INSTALLED_DIR}/lib/)
set(ISPFEC_LIB IspFec)

if (NOT libdrm_FOUND)
    set(ISPFEC_INCLUDE_DIR
        ${ISPFEC_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/sample/deps/include
        )
endif()

if (ENABLE_SAMPLE_ISPFEC)
    set (SAMPLE_FEC_SRCS
         ${CMAKE_SOURCE_DIR}/sample/sample_fec.c
         ${CMAKE_SOURCE_DIR}/sample/sample_drm_buffer.c
        )

    include_directories(sample ${ISPFEC_INCLUDE_DIR})
    link_directories("${ISPFEC_LIB_DIR}" "${LOCAL_DRM_LIB_DIR}")

    add_executable(sample_fec ${SAMPLE_FEC_SRCS})
    target_link_libraries(sample_fec ${ISPFEC_LIB} ${LOCAL_DRM_LIB})

    install(TARGETS sample_fec RUNTIME DESTINATION bin)
endif()

if (ENABLE_SAMPLE_FEC_FOR_AVSP)
    set (SAMPLE_FOR_AVSP_FEC_SRCS
         ${CMAKE_SOURCE_DIR}/sample/sample_fec_for_avsp.c
         ${CMAKE_SOURCE_DIR}/sample/sample_drm_buffer.c
        )

    link_directories("${LOCAL_DRM_LIB_DIR}")

    add_executable(sample_fec_for_avsp ${SAMPLE_FOR_AVSP_FEC_SRCS})
    target_link_libraries(sample_fec_for_avsp ${LOCAL_DRM_LIB})

    install(FILES ${CMAKE_SOURCE_DIR}/include_prv/rk-fec-config.h DESTINATION include/IspFec)
    install(TARGETS sample_fec_for_avsp RUNTIME DESTINATION bin)
endif()