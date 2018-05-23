#by peter
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := tests
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := parser_testcase.c 

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../common\
        $(LOCAL_PATH)/../hls_main\
        $(LOCAL_PATH)/../downloader\
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../../../amavutils/include \
        $(LOCAL_PATH)/../../../amffmpeg

LOCAL_MODULE := m3uparser_test 
LOCAL_STATIC_LIBRARIES := libhls libhls_http libhls_common 

LOCAL_SHARED_LIBRARIES :=libamplayer libcutils libssl libamavutils libcrypto
ifeq ($(BUILD_WITH_VIEWRIGHT_WEB), true)
 LOCAL_CFLAGS += -DENABLE_VIEWRIGHT_WEB
endif
LOCAL_SHARED_LIBRARIES +=libdl 
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := tests
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := hls_merge_tool.c 

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../common\
        $(LOCAL_PATH)/../include \
        $(LOCAL_PATH)/../hls_main \
        $(LOCAL_PATH)/../../../amavutils/include \
        $(LOCAL_PATH)/../../../amffmpeg

LOCAL_MODULE := hls_merge_tool 
LOCAL_STATIC_LIBRARIES := libhls libhls_http libhls_common 

LOCAL_SHARED_LIBRARIES :=libamplayer libcutils libssl libamavutils libcrypto


ifeq ($(BUILD_WITH_VIEWRIGHT_WEB), true)
 LOCAL_CFLAGS += -DENABLE_VIEWRIGHT_WEB
endif
LOCAL_SHARED_LIBRARIES +=libdl 

include $(BUILD_EXECUTABLE)

