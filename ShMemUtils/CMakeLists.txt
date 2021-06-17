cmake_minimum_required(VERSION 3.5)

project(ShMemUtilsLib)

############################################################
# Create a library
############################################################

#Generate the static library from the library sources
set(SOURCE_FILES
        src/ShMemUtils.cpp
)

set(HEADER_FILES
        include/ShMemUtils.h
)


add_library(ShMemUtils STATIC 
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

target_include_directories(ShMemUtils
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)

