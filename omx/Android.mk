LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GST_MAJORMINOR := 0.10

LOCAL_SRC_FILES := \
    gstomx.c                \
    gstomx_base_filter.c    \
    gstomx_interface.c      \
    gstomx_base_videodec.c  \
    gstomx_util.c           \
    gstomx_dummy.c          \
    gstomx_aacdec.c         \
    gstomx_amrnbdec.c       \
    gstomx_amrwbdec.c       \
    gstomx_mp3dec.c         \
    gstomx_h263dec.c        \
    gstomx_h264dec.c        \
    gstomx_mpeg4dec.c

LOCAL_SHARED_LIBRARIES := \
    libdl                   \
    libgstbase-0.10         \
    libgstreamer-0.10       \
    libglib-2.0             \
    libgthread-2.0          \
    libgmodule-2.0          \
    libgobject-2.0

LOCAL_WHOLE_STATIC_LIBRARIES := libgstomxutil

LOCAL_MODULE := libgstomx

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)                       \
    $(LOCAL_PATH)/headers               \
    $(GST_OPENMAX_TOP)/android          \
    $(GST_OPENMAX_TOP)/util/            \
    external/gstreamer                  \
    external/gstreamer/android          \
    external/gstreamer/libs             \
    external/gstreamer/gs               \
    external/gstreamer/gst/android      \
    external/glib                       \
    external/glib/android               \
    external/glib/glib                  \
    external/glib/glib/android          \
    external/glib/gmodule               \
    external/glib/gobject               \
    external/glib/gthread

LOCAL_CFLAGS := \
    -DHAVE_CONFIG_H

include $(BUILD_PLUGIN_LIBRARY)
