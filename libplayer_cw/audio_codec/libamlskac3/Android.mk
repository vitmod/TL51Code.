LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)


LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc 
LOCAL_MODULE    := libamlskac3
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH)  $(LOCAL_PATH)/../../amadec/include $(LOCAL_PATH)/../../amadec/

LOCAL_REQUIRED_MODULES := $(patsubst %.so,%,$(filter %.so,$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
  find . -name "*.so" -and -not -name ".*"))))


include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := $(filter %.so,$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
  find . -name "*.so" -and -not -name ".*")))

include $(BUILD_MULTI_PREBUILT)

