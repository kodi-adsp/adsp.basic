project(adsp.basic)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR})

set(BASIC_SOURCES src/AudioDSPBasic.cpp
                  src/GUIDialogSpeakerDistance.cpp
                  src/addon.cpp
                  src/Process_Stereo/DSPProcessStereo.cpp
                  src/GUIDialogSpeakerGain.cpp
                  src/DSPProcessMaster.cpp
                  src/AudioDSPSettings.cpp
                  src/filter/high_shelf.cpp
                  src/filter/delay.cpp
                  src/filter/complex.cpp
                  src/filter/filter.cpp
                  src/filter/mkfilter.cpp
                  src/AudioDSPSoundTest.cpp)

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${p8-platform_LIBRARIES})

build_addon(adsp.basic BASIC DEPLIBS)

add_definitions(-DADSP_BASIC_VERSION="${BASIC_VERSION}")

include(CPack)
