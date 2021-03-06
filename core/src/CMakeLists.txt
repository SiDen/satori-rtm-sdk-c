# Construct sources list
set(BASE_SOURCES rtm.c rtm_json.c rtm_byteorder.c)
set(SOURCES ${BASE_SOURCES})
file(GLOB BASE_HEADERS *.h)


if(UNIX)
  list(APPEND SOURCES io/rtm_posix.c)
elseif(MSVC)
  list(APPEND SOURCES io/rtm_windows.c)
else()
  message(FATAL_ERROR "Environment that is neither UNIX nor Visual Studio is not supported")
endif()

if(USE_GNUTLS)
  list(APPEND SOURCES ssl/rtm_gnutls.c)
elseif(USE_OPENSSL)
  list(APPEND SOURCES ssl/rtm_openssl.c ssl/rtm_openssl_bio.c)
elseif(USE_APPLE_SSL)
  list(APPEND SOURCES ssl/rtm_apple_ssl.c)
endif()

# Sources list is now ready
if(BUILD_STATIC)
    add_library(rtm-core-sdk STATIC ${SOURCES})
    set(RTM_BUILD_STATIC 1)
else()
    add_library(rtm-core-sdk SHARED ${SOURCES})
    set(RTM_BUILD_STATIC 0)
endif()

set_target_properties(rtm-core-sdk PROPERTIES LINKER_LANGUAGE C)

if(MSVC)
  target_link_libraries(rtm-core-sdk wsock32 ws2_32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories("$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>")
target_link_libraries(rtm-core-sdk ${TLS_LIBRARIES})

# Create configuration header
include(CheckFunctionExists)
check_function_exists(getenv RTM_HAS_GETENV)
include(CheckCSourceCompiles)
check_c_source_compiles("#include <stdio.h>\nint main() { fprintf(stderr, \".\"); fflush(stderr); return 0; }" RTM_HAS_FIO)
set(RTM_LOGGING ${ENABLE_LOGGING})

configure_file(rtm_config.h.in
  rtm_config.h
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rtm_config.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")


# Installation
if(OSX)
    if(BUILD_STATIC)
      set_target_properties(rtm-core-sdk PROPERTIES
                            OUTPUT_NAME "SatoriCoreSDKtatic"
                            INSTALL_NAME_DIR "@rpath/SatoriCoreSDKStatic.framework"
                            INSTALL_RPATH "@executable_path/Frameworks;@loader_path/Frameworks"
                            )
      install(FILES rtm.h
              DESTINATION include/SatoriCoreSDKStatic
              RENAME Core.h
              )
  else()
      set_target_properties(rtm-core-sdk PROPERTIES
                            OUTPUT_NAME "SatoriCoreSDK"
                            INSTALL_NAME_DIR "@rpath/SatoriCoreSDK.framework"
                            INSTALL_RPATH "@executable_path/Frameworks;@loader_path/Frameworks"
                            )
      install(FILES rtm.h
              DESTINATION include/SatoriCoreSDK
              RENAME Core.h
              )
  endif()
endif()

install(FILES rtm.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(TARGETS rtm-core-sdk
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# Arduino SDK
if(ARDUINO_SDK)
  set(ARDUINO_DIR ${CMAKE_CURRENT_BINARY_DIR}/arduino)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../library.properties.in ${ARDUINO_DIR}/library.properties @ONLY)

  function(ArduinoConfig)
    set(RTM_HAS_GETENV 0)
    set(RTM_HAS_FIO 0)
    set(RTM_LOGGING 0)
    set(RTM_USE_GNUTLS 0)
    set(RTM_USE_OPENSSL 0)
    set(RTM_USE_APPLE_SSL 0)
    configure_file(rtm_config.h.in ${ARDUINO_DIR}/rtm_config.h @ONLY)
  endfunction()
  ArduinoConfig()

  set(ARDUINO_FILES ${ARDUINO_DIR}/rtm_config.h ${ARDUINO_DIR}/library.properties ${CMAKE_CURRENT_SOURCE_DIR}/ssl/rtm_ssl_cert.c ${CMAKE_CURRENT_SOURCE_DIR}/io/rtm_esp_arduino.cpp ${CMAKE_CURRENT_SOURCE_DIR}/rtm_hmac_md5.c ${BASE_SOURCES} ${BASE_HEADERS})
  set(ARDUINO_TARGET ${CMAKE_BINARY_DIR}/satori_esp_arduino_library.zip)
  add_custom_command(
    OUTPUT ${ARDUINO_TARGET}
    COMMAND rm -f ${ARDUINO_TARGET}
    COMMAND zip -j ${ARDUINO_TARGET} ${ARDUINO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ARDUINO_FILES}
  )

  add_custom_target(arduino-lib ALL DEPENDS ${ARDUINO_TARGET})
endif()
