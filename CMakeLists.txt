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
set(SOURCES
    src/view/wincapture.cpp
    test/wincapture_test.cpp      # ✅ 加上测试文件！
)

set(HEADERS
    include/common.h
    include/wincapture.h
)

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