cmake_minimum_required(VERSION 2.6)

project(regions_assembler)
find_package(Boost COMPONENTS program_options REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CURRENT_BINARY_DIR "../bin")
set(CMAKE_INSTALL_PREFIX "./")

#add_subdirectory(tests)
add_subdirectory(utils)
add_subdirectory(model)
add_subdirectory(misc)
#add_subdirectory(gtest-1.6.0)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(/opt/local/include)
include_directories(../include/misc)
include_directories(../include/model)
include_directories(../include/utils)

link_directories(${Boost_LIBRARY_DIRS})
link_directories(/opt/local/lib)
link_directories(${CMAKE_SOURCE_DIR}/misc)
link_directories(${CMAKE_SOURCE_DIR}/model)
link_directories(${CMAKE_SOURCE_DIR}/utils)


add_executable(regions_assembler
	main.cpp)

target_link_libraries(regions_assembler log4cxx misc model utils ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

install(TARGETS regions_assembler DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(DIRECTORY "./config"
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        FILES_MATCHING 
        PATTERN "*.ini")
        
install(DIRECTORY "./test_data"
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        FILES_MATCHING 
        PATTERN "*.txt")
        
