#
# TFC command-line client
#

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -pthread)

# create binary project
file(GLOB SRC_FILES src/*.cpp)
file(GLOB HEADER_FILES include/*.h)
add_executable(tfc-cli ${SRC_FILES} ${HEADER_FILES})

# add includes
target_include_directories(tfc-cli PRIVATE include)
target_link_libraries(tfc-cli PRIVATE tasker PRIVATE tfc)
