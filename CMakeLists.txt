cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Perception_Agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("/utf-8")
#输出
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# link附加库
set(OpenCV_DIR "D:/CodingPrograms/Packages/opencv/build")
find_package(OpenCV REQUIRED)

# link源代码（主程序源文件 + 测试文件）
set(TEST_TARGET "2")  # 改这里切换

# ---------- 公共源文件 ----------
set(COMMON_SOURCES
    src/view/wincapture.cpp
    src/view/mainpartfinder.cpp
    src/view/finalouter.cpp
)

set(COMMON_HEADERS
    include/common.h
    include/wincapture.h
    include/mainpartfinder.h
    include/finalouter.h
)


if(TEST_TARGET STREQUAL "1")
    set(SOURCES ${COMMON_SOURCES} test/wincapture_test.cpp)
    set(HEADERS ${COMMON_HEADERS})
    set(TARGET_NAME "WinCapture_Test")
elseif(TEST_TARGET STREQUAL "2")
    set(SOURCES ${COMMON_SOURCES} test/viewpart_text.cpp)
    set(HEADERS ${COMMON_HEADERS})
    set(TARGET_NAME "WinCapture_Test")



#elseif(TEST_TARGET STREQUAL "2")
#    set(SOURCES ${COMMON_SOURCES} test/mainpartfinder_test.cpp)
#    set(HEADERS ${COMMON_HEADERS})
#    set(TARGET_NAME "MainPartFinder_Test")

endif()

# 生成可执行文件
add_executable(Perception_Agent ${SOURCES} ${HEADERS})

target_include_directories(Perception_Agent
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(Perception_Agent
    ${OpenCV_LIBS}
    gdi32
    user32
)