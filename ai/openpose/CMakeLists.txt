cmake_minimum_required(VERSION 2.8.12)

PROJECT(openpose)

set(CMAKE_CXX_STANDARD 11)

set(OpenCV_DIR ../third_party/opencv/build)
find_package( OpenCV REQUIRED )

set(gflags_DIR ../third_party/gflags/build)
find_package(gflags REQUIRED)

set(glog_DIR ../third_party/glog/build)
find_package(glog REQUIRED)


include_directories( ${OpenCV_INCLUDE_DIRS})

MACRO(add_example name)
  ADD_EXECUTABLE(${name} ${name}.cpp)
  TARGET_LINK_LIBRARIES(${name} gflags glog::glog ${OpenCV_LIBS})
ENDMACRO()

add_example(openpose)
