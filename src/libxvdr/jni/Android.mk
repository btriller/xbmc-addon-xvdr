LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := xvdr
LOCAL_SRC_FILES := \
	../src/libTcpSocket/linux/net_posix.c \
	../src/iso639.cpp \
	../src/requestpacket.cpp \
	../src/tools.cpp \
	../src/XVDRCallbacks.cpp \
	../src/XVDRData.cpp \
	../src/XVDRDemux.cpp \
	../src/XVDRRecording.cpp \
	../src/XVDRResponsePacket.cpp \
	../src/XVDRSession.cpp \
	../src/XVDRThread.cpp

LOCAL_CFLAGS    := -I$(LOCAL_PATH)/../include -DUSE_DEMUX=1
LOCAL_LDLIBS    := -lz

include $(BUILD_SHARED_LIBRARY)
