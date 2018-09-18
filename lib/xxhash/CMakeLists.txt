#
# xxhash
#

set(CMAKE_C_STANDARD 99)

# create binary project
file(GLOB SRC_FILES src/*.c)
file(GLOB HEADER_FILES xxhash/*.h)
add_library(xxhash STATIC ${SRC_FILES} ${HEADER_FILES})

# add includes
target_include_directories(xxhash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
