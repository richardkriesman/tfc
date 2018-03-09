cmake_minimum_required(VERSION 3.9)
project(tfc VERSION 0.0.1 LANGUAGES CXX C)

# language options
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -m64)

# output options
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# sources
set(HEADERS include/TfcFile.h include/TfcFileException.h include/Record.h include/TagTable.h include/BlobTable.h
        include/Terminal.h)
set(SOURCES src/main.cpp src/TfcFile.cpp src/TfcFileException.cpp src/Record.cpp src/TagTable.cpp src/BlobTable.cpp)

# picosha2
add_library(picosha2 include/picosha2.h)
set_target_properties(picosha2 PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(picosha2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# xxhash
add_subdirectory(lib/xxhash/cmake_unofficial)

# primary build target
add_executable(tfc ${HEADERS} ${SOURCES})
target_link_libraries(tfc xxhash)
target_include_directories(tfc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(tfc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_compile_definitions(tfc PRIVATE VERSION=\"${PROJECT_VERSION}\")
