cmake_minimum_required(VERSION 3.23)
project(CLionProjects)

set(CMAKE_CXX_STANDARD 20)

add_executable(CLionProjects main.cpp
        main.cpp)

add_subdirectory(libraries/fmt)
add_subdirectory(libraries/glfw-master)
add_subdirectory(libraries/glew-2.1.0)
add_subdirectory(libraries/glm-master)
add_subdirectory(libraries/rapidxml-master)

set(GLEW_INCLUDE_DIRS "libraries/glew-2.1.0/include")
set(GLEW_LIBRARIES "libraries/glew-2.1.0")

target_link_libraries(CLionProjects fmt::fmt)
target_link_libraries(CLionProjects glfw)
target_link_libraries(CLionProjects glew_s)
target_link_libraries(CLionProjects glm)
target_link_libraries(CLionProjects GTest::gtest_main)