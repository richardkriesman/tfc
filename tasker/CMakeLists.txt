#
# Tasker module
#

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -pthread)

# create binary project
file(GLOB SRC_FILES src/*.cpp)
file(GLOB HEADER_FILES tasker/*.h)
add_library(tasker STATIC ${SRC_FILES} ${HEADER_FILES})

# add includes
target_include_directories(tasker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
