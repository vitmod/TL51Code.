LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	codec/codec_ctrl.c \
	codec/codec_h_ctrl.c \
	codec/codec_msg.c \
	audio_ctl/audio_ctrl.c \
	amsub_ctl/amsub_ctr.c\
	codec/codec_info.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
										$(LOCAL_PATH)/codec \
										$(LOCAL_PATH)/audio_ctl \
										$(LOCAL_PATH)/amsub_ctl \
										$(LOCAL_PATH)/../amadec/include \
										$(LOCAL_PATH)/../amavutils/include \
										$(LOCAL_PATH)/../amsubdec/
										
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES := libamadec
LOCAL_MODULE:= libamcodec

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	codec/codec_ctrl.c \
	codec/codec_h_ctrl.c \
	codec/codec_msg.c \
	audio_ctl/audio_ctrl.c \
	amsub_ctl/amsub_ctr.c \
	codec/codec_info.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	$(LOCAL_PATH)/codec \
	$(LOCAL_PATH)/audio_ctl \
	$(LOCAL_PATH)/amsub_ctl \
	$(LOCAL_PATH)/../amadec/include \
	$(LOCAL_PATH)/../amavutils/include \
	$(LOCAL_PATH)/../amsubdec/



LOCAL_STATIC_LIBRARIES := libamadec lib_aml_agc
LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc libamavutils libamsubdec

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libamcodec
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

