# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project (ClientTest)

#set(CMAKE_BUILD_TYPE Debug)

# Create a sources variable with a link to all cpp files to compile
set(SOURCE_FILES
    src/ShMemClient.cpp
    src/ClientTest.cpp
)

set(HEADER_FILES
	include/ShMemClient.h
)

set(COMMON_INCLUDE
        ${PROJECT_SOURCE_DIR}../ShMemHolder/include
)

# Add an executable with the above sources
add_executable(ClientTest 
	${SOURCE_FILES}
	${HEADER_FILES}
)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
target_include_directories(ClientTest
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
        $(COMMON_INCLUDE)
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# link the new library target with the binary target
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux") 
target_link_libraries(ClientTest
    PRIVATE 
    ShMemUtils
    Threads::Threads
    rt)
else()
target_link_libraries(ClientTest
    PRIVATE 
    ShMemUtils)
endif()
