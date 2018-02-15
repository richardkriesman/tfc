cmake_minimum_required(VERSION 3.9)
project(tfc)

set(CMAKE_CXX_STANDARD 11)

add_executable(tfc main.cpp libtfc/TfcFile.cpp libtfc/TfcFile.h libtfc/picosha2/picosha2.h libtfc/TfcFileException.h libtfc/TfcFileException.cpp libtfc/JumpTable.cpp libtfc/JumpTable.h)