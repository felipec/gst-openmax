LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        async_queue.c

LOCAL_CFLAGS :=

LOCAL_C_INCLUDES :=                     \
        $(LOCAL_PATH)                   \
        $(GST_OPENMAX_TOP)/android      \
        external/gstreamer/libs         \
        external/gstreamer/gst          \
        external/gstreamer/gst/android  \
        external/glib/glib              \
        external/glib/glib/android      \
        external/glib/gmodule           \
        external/glib/gobject           \
        external/glib/gthread           \
        external/glib                   \
        external/glib/android           \
        external/gstreamer              \
        external/gstreamer/android

LOCAL_MODULE := libgstomxutil

include $(BUILD_STATIC_LIBRARY)
