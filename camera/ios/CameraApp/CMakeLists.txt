cmake_minimum_required(VERSION 3.14)
project(CameraApp LANGUAGES C CXX OBJC OBJCXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认目标平台为真机
if(NOT DEFINED SIMULATOR OR SIMULATOR STREQUAL "OFF")
    set(CMAKE_OSX_SYSROOT iphoneos)  # 真机构建
    set(CMAKE_OSX_ARCHITECTURES "arm64")  # 仅针对真机
    message(STATUS "Building for iPhone device (iphoneos).")
else()
    set(CMAKE_OSX_SYSROOT iphonesimulator)  # 模拟器构建
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")  # 同时支持模拟器和真机架构
    message(STATUS "Building for iPhone simulator (iphonesimulator).")
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")  # 设置为您的设备支持的最低iOS版本

# 设置源文件列表
set(SOURCE_FILES
    main.m
    AppDelegate.h
    AppDelegate.m
    CameraViewController.h
    CameraViewController.m
)

# 创建应用程序目标
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 指定构建的目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE YES                          # 设置为YES以构建iOS应用
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist # 指定Info.plist文件
)

# 链接 UIKit 和 AVFoundation 框架
target_link_libraries(${PROJECT_NAME} 
    "-framework UIKit" 
    "-framework AVFoundation"
    "-framework Foundation"
)

# 设置应用的设备族：1表示iPhone，2表示iPad
set_target_properties(${PROJECT_NAME} PROPERTIES
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"  # 支持 iPhone 和 iPad
)
