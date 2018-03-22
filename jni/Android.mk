LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libtfc
LOCAL_CPPFLAGS := -std=c++11 -fexceptions -pthread
LOCAL_C_INCLUDES += \
    include/ \
    lib/
LOCAL_SRC_FILES += \
    extensions.cpp \
	../src/main.cpp \
	../src/libtfc/TfcFile.cpp \
	../src/libtfc/TfcFileException.cpp \
	../src/libtfc/Record.cpp \
    ../src/libtfc/TagTable.cpp \
    ../src/libtfc/BlobTable.cpp \
    ../src/libtfc/Journal.cpp \
    ../src/tasker/Looper.cpp \
    ../src/tasker/TaskHandle.cpp \
    ../src/tasker/TaskException.cpp \
    ../src/tasker/Task.cpp \
    ../lib/xxhash/xxhash.c
include $(BUILD_EXECUTABLE)
