# The MIT License (MIT)
#
# Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.2)

###
# project
###
set(PROJECT cpp_redis_tests)
project(${PROJECT} CXX)

find_package(OpenCV REQUIRED)


###
# compilation options
###
if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(NOT WIN32)

# Enable ExternalProject CMake module
include(ExternalProject)

# Download and install GoogleTest
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

# Set libgmock properties
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)

# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/googletest/include"
                    "${source_dir}/googlemock/include")

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined -shared -fPIC")


add_definitions(-DOPENCV)
add_definitions(-DCV_MAJOR_VERSION=${OpenCV_VERSION_MAJOR})
add_definitions(-DCV_IGNORE_DEBUG_BUILD_GUARD)

list(APPEND OPENCV_LINKED_LIBS opencv_core opencv_highgui opencv_imgproc)

if( OpenCV_VERSION_MAJOR GREATER 2 )
    list( APPEND OPENCV_LINKED_LIBS ${OpenCV_LIBS} opencv_videoio opencv_imgcodecs )
endif()


###
# includes
###
include_directories(${DEPS_INCLUDES} ${CPP_REDIS_INCLUDES})


###
# libraries
###
link_directories(${DEPS_LIBRARIES})


###
# executables
###
set(MAIN "sources/main.cpp")
set(DIRS "sources/spec" "sources/spec/**")
foreach(DIR ${DIRS})
  file(GLOB s_${DIR} "${DIR}/*.cpp")
  foreach(SOURCE ${s_${DIR}})
    get_filename_component(TEST_NAME ${SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${MAIN} ${SOURCE})
    target_link_libraries(${TEST_NAME} cpp_redis libgtest ${OPENCV_LINKED_LIBS} tacopie)
    if(WIN32)
      target_link_libraries(${TEST_NAME} ws2_32)
    else()
      target_link_libraries(${TEST_NAME} pthread)
    endif(WIN32)
    add_test(NAME ${TEST_NAME}_tests COMMAND ${TEST_NAME})
  endforeach()
endforeach()
