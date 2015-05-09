project(libsamplerate)

cmake_minimum_required(VERSION 2.8)

if(WIN32)
  set(SOURCES src/samplerate.c
              src/src_linear.c
              src/src_sinc.c
              src/src_zoh.c)

  add_definitions(-DWIN32 -D_LIB)

  add_library(libsamplerate ${SOURCES})

  include_directories( ${PROJECT_SOURCE_DIR}/src
                       ${PROJECT_SOURCE_DIR}/Win32)
              
  include_directories( ${PROJECT_SOURCE_DIR}/src
                       ${PROJECT_SOURCE_DIR}/Win32)

  set(HEADERS ${PROJECT_SOURCE_DIR}/src/samplerate.h
              ${PROJECT_SOURCE_DIR}/src/float_cast.h
              ${PROJECT_SOURCE_DIR}/src/common.h
              ${PROJECT_SOURCE_DIR}/Win32/config.h)

  install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/samplerate)

  install(TARGETS libsamplerate DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
else()
  include(ExternalProject)
  externalproject_add(libsamplerate
            SOURCE_DIR ${CMAKE_SOURCE_DIR}
            UPDATE_COMMAND ""
            CONFIGURE_COMMAND <SOURCE_DIR>/configure 
            --prefix=${OUTPUT_DIR}
            --enable-static
            --disable-shared
            "CFLAGS=-fPIC -DPIC -O2"
            INSTALL_COMMAND ""
            BUILD_IN_SOURCE 1)

  install(CODE "execute_process(COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
endif()