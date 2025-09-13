cmake_minimum_required(VERSION 3.16)
project(dronecan_moteus_bridge LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)

### libcanard
add_subdirectory(external/libcanard)

### SocketCAN driver
add_library(socketcan_driver STATIC
  external/libcanard/drivers/socketcan/socketcan.c
)
target_include_directories(socketcan_driver PRIVATE
  ${CMAKE_SOURCE_DIR}/external/libcanard
)
target_include_directories(socketcan_driver PUBLIC
  ${CMAKE_SOURCE_DIR}/external/libcanard/drivers/socketcan
)

### DSDL generation
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(DSDL_DIR ${CMAKE_SOURCE_DIR}/external/DSDL)
set(DSDLC_PY ${CMAKE_SOURCE_DIR}/external/dronecan_dsdlc/dronecan_dsdlc.py)
set(DSDL_GEN_DIR ${CMAKE_BINARY_DIR}/dsdl_generated)

set(VENV_DIR ${CMAKE_BINARY_DIR}/venv)
set(REQUIREMENTS_ARGS
    -r ${CMAKE_SOURCE_DIR}/external/dronecan_dsdlc/requirements.txt
    pydronecan
)
set(VENV_PYTHON ${VENV_DIR}/bin/python)

execute_process(
  COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  RESULT_VARIABLE RV1
)
if(NOT RV1 EQUAL 0)
  message(FATAL_ERROR "Failed to create venv")
endif()

execute_process(
  COMMAND ${VENV_DIR}/bin/pip install --upgrade pip
  RESULT_VARIABLE RV2
)
if(NOT RV2 EQUAL 0)
  message(FATAL_ERROR "Failed to upgrade pip in venv")
endif()

execute_process(
  COMMAND ${VENV_DIR}/bin/pip install ${REQUIREMENTS_ARGS}
  RESULT_VARIABLE RV3
)
if(NOT RV3 EQUAL 0)
  message(FATAL_ERROR "Failed to install requirements in venv")
endif()

file(MAKE_DIRECTORY ${DSDL_GEN_DIR})
execute_process(
  COMMAND ${VENV_PYTHON} ${DSDLC_PY} -O ${DSDL_GEN_DIR}
          ${DSDL_DIR}/dronecan
          ${DSDL_DIR}/uavcan
          ${DSDL_DIR}/com
          ${DSDL_DIR}/ardupilot
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  RESULT_VARIABLE DSDL_GEN_RV
)

if(NOT DSDL_GEN_RV EQUAL 0)
  message(FATAL_ERROR "dronecan_dsdlc failed with code ${DSDL_GEN_RV}")
endif()

# Compile all generated C sources into a small static lib
file(GLOB DSDL_C_SRCS CONFIGURE_DEPENDS ${DSDL_GEN_DIR}/src/*.c)

add_library(dsdl_generated STATIC ${DSDL_C_SRCS})
target_include_directories(dsdl_generated
  PUBLIC  ${DSDL_GEN_DIR}/include
  PRIVATE ${CMAKE_SOURCE_DIR}/external/libcanard
)
target_link_libraries(dsdl_generated PUBLIC canard_tgt)


### Main executable
add_executable(${PROJECT_NAME}
    src/canard_interface.cpp
    src/main.cpp
    src/moteus_interface.cpp
    src/tunnel_node.cpp
    src/util.cpp
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        DRONECAN_CXX_WRAPPERS
        NUM_CANARD_HANDLERS=16
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Link against libcanard's CMake target
target_link_libraries(${PROJECT_NAME}
    PRIVATE 
        canard_tgt
        socketcan_driver
        dsdl_generated
)
