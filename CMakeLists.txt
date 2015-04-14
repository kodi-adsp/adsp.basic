project(adsp.basic)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(SampleRate REQUIRED)
#find_package(FFTW3 REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    #${FFTW3_INCLUDE_DIRS}
                    ${SAMPLERATE_INCLUDE_DIRS})

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
            #${FFTW3_LIBRARIES}
            ${SAMPLERATE_LIBRARIES})

build_addon(adsp.basic BASIC DEPLIBS)

add_definitions(-DADSP_BASIC_VERSION="${BASIC_VERSION}")

include(CPack)
