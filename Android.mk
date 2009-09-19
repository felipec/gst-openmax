LOCAL_PATH := $(call my-dir)

GST_OPENMAX_TOP := $(LOCAL_PATH)

include $(CLEAR_VARS)

include $(GST_OPENMAX_TOP)/util/Android.mk
include $(GST_OPENMAX_TOP)/omx/Android.mk
