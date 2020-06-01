cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-fsanitize=address -Werror -Wextra")

project(shttp-server)

find_package(Threads)

set(FILES_SOURCE
        server.cpp)
set(FILES_INCLUDE
        utils.h
        base.h fileio.h headercontainer.h)

add_executable(${PROJECT_NAME}
        ${FILES_INCLUDE}
        ${FILES_SOURCE})

target_link_libraries(${PROJECT_NAME}
        Threads::Threads)
