cmake_minimum_required(VERSION 2.8.12)

PROJECT(omniplay)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(CMAKE_CXX_STANDARD 11)

find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

find_package(X11 REQUIRED)
set(LIBS ${GLFW3_LIBRARY} GL X11 pthread Xrandr Xinerama Xi Xxf86vm Xcursor freetype)

add_library(GLAD "glad.c")
set(LIBS ${LIBS} GLAD)
set(LIBS ${LIBS} dl)

include_directories(${CMAKE_SOURCE_DIR}/includes)

MACRO(add_example name)
  ADD_EXECUTABLE(${name} ${name}.cpp)
  TARGET_LINK_LIBRARIES(${name} ${LIBS})
ENDMACRO()

add_example(omniplay)
