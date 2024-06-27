cmake_minimum_required(VERSION 3.13)

# Toolchain must be set before project() call.
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/coralmicro/cmake/toolchain-arm-none-eabi-gcc.cmake)
endif()

project(coralmicro-app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(coralmicro)
add_subdirectory(coralmicro)

# set(YAMNET_CPU true) # To run YamNet on the CPU, uncomment.
if(YAMNET_CPU)
    add_compile_definitions(YAMNET_CPU)
    set(model_name yamnet_spectra_in.tflite)
else()
    set(model_name yamnet_spectra_in_edgetpu.tflite)
endif()



set(main_m7_SOURCES
        main_cm7.cc
        src/app/rpc/RpcServer.cpp
        src/app/ml/InferenceFactory.cpp
        src/app/ml/AlarmInterpreter.cpp
        src/app/ml/AudioBridge.cpp
        src/app/thread/ThreadInference.cpp
        src/app/thread/QueueMainM7.cpp
        src/app/thread/QueueMain.cpp

        DATA
        ${CMAKE_SOURCE_DIR}/coralmicro/models/yamnet_test_audio.bin
        ${CMAKE_SOURCE_DIR}/coralmicro/models/${model_name}
    )


add_executable_m7(coralmicro-app
        ${main_m7_SOURCES}
)

target_link_libraries(coralmicro-app
        libs_base-m7_freertos
        libs_rpc_http_server
        libs_rpc_utils
)
