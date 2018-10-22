#
# tfc
#

set(CMAKE_CXX_STANDARD 11)

# create binary project
file(GLOB SRC_FILES src/*.cpp)
file(GLOB HEADER_FILES tfc/*.h)
add_library(tfc STATIC ${SRC_FILES} ${HEADER_FILES})

# create shared object library
add_library(tfc-shared SHARED ${SRC_FILES} ${HEADER_FILES})

# add includes
target_include_directories(tfc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(tfc-shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# add dependencies
target_link_libraries(tfc PRIVATE xxhash)
target_link_libraries(tfc-shared PRIVATE xxhash)
