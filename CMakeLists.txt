cmake_minimum_required(VERSION 3.9)
project(tfc VERSION 0.0.1 LANGUAGES CXX C)

# language options
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -pthread")

# output options
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin/linux)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# sources
set(SOURCES src/main.cpp src/libtfc/TfcFile.cpp src/libtfc/TfcFileException.cpp src/libtfc/Record.cpp
        src/libtfc/TagTable.cpp src/libtfc/BlobTable.cpp src/libtfc/Journal.cpp src/tasker/Looper.cpp
        src/tasker/TaskHandle.cpp src/tasker/TaskException.cpp
        src/tasker/Task.cpp include/Tasker.h include/libtfc/TfcFile.h
        include/libtfc/TfcFileException.h include/libtfc/Record.h include/libtfc/TagTable.h
        include/libtfc/BlobTable.h include/Terminal.h include/License.h include/libtfc/Journal.h
        include/Tasker.h src/tasker/Event.cpp)

# xxhash
add_subdirectory(lib/xxhash/cmake_unofficial)

# primary build target
add_executable(tfc ${SOURCES})
target_link_libraries(tfc PRIVATE xxhash)
target_include_directories(tfc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(tfc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_compile_definitions(tfc PRIVATE VERSION=\"${PROJECT_VERSION}\")
