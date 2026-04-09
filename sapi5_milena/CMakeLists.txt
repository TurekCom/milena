cmake_minimum_required(VERSION 3.20)
project(MilenaSapi5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SPEECHSDK51_ROOT
  "${CMAKE_CURRENT_SOURCE_DIR}/../tools/SpeechSDK51_admin/program files/Microsoft Speech SDK 5.1"
)

if(NOT EXISTS "${SPEECHSDK51_ROOT}/Include/sapi.h")
  set(SPEECHSDK51_ROOT
    "C:/Users/turek/Desktop/blackbox/tools/SpeechSDK51_admin/program files/Microsoft Speech SDK 5.1"
  )
endif()

if(NOT EXISTS "${SPEECHSDK51_ROOT}/Include/sapi.h")
  message(FATAL_ERROR "Nie znaleziono Speech SDK 5.1 headers.")
endif()

add_library(MilenaSapi5 SHARED
  engine/milena_sapi5.cpp
  engine/milena_sapi5.def
)

target_compile_definitions(MilenaSapi5 PRIVATE
  UNICODE
  _UNICODE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
)

target_include_directories(MilenaSapi5 PRIVATE
  "${SPEECHSDK51_ROOT}/Include"
)

target_compile_options(MilenaSapi5 PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHsc /utf-8>
)

target_link_libraries(MilenaSapi5 PRIVATE
  ole32
  uuid
)

set_target_properties(MilenaSapi5 PROPERTIES
  OUTPUT_NAME "MilenaSapi5"
)
