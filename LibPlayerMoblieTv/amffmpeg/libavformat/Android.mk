LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk
LOCAL_SRC_FILES := $(FFFILES)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
	$(LOCAL_PATH)/../../amavutils/include/	\
	$(LOCAL_PATH)/../../third_parts/rtmpdump	\
	$(LOCAL_PATH)/../../third_parts/udrm	\
	$(LOCAL_PATH)/../../amplayer/player/include/ \
	$(LOCAL_PATH)/../../amcodec/include/ \
	external/zlib
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_MODULE := $(FFNAME)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk
LOCAL_SRC_FILES := $(FFFILES)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
	$(LOCAL_PATH)/../../amavutils/include/	\
	$(LOCAL_PATH)/../../third_parts/rtmpdump	\
	$(LOCAL_PATH)/../../third_parts/udrm	\
	$(LOCAL_PATH)/../../amplayer/player/include/ \
	$(LOCAL_PATH)/../../amcodec/include/ \
	external/zlib
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_MODULE := $(FFNAME)
LOCAL_SHARED_LIBRARIES += librtmp  libutils libmedia libz libbinder libdl libcutils libc libavutil libavcodec libamavutils libudrm
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

# Reset CC as it's overwritten by common.mk
CC := $(HOST_CC)
