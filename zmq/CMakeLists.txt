cmake_minimum_required(VERSION 3.27)
project(zmq)

set(CMAKE_CXX_STANDARD 17)

find_library(ZMQ_LIBRARY NAMES zmq)
find_library(JSONCPP_LIB NAMES jsoncpp)
find_package(OpenCV REQUIRED)

add_executable(zmq main.cpp)
target_link_libraries(zmq ${ZMQ_LIBRARY})
target_link_libraries(zmq ${JSONCPP_LIB})
target_link_libraries(zmq ${OpenCV_LIBS})