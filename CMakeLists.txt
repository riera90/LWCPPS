cmake_minimum_required (VERSION 3.9)
include(GNUInstallDirs)

project(lwcpps VERSION 1.0.0 DESCRIPTION "LightWeight C++ Server")

set(CMAKE_BUILD_TYPE Release)

set(SOURCES
        lwcpps.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES lwcpps
        DESTINATION /usr/include
)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER lwcpps)

target_include_directories(${PROJECT_NAME} PRIVATE .)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})

find_package(Threads)
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

target_compile_definitions (${PROJECT_NAME} PRIVATE -DDEBUG)
