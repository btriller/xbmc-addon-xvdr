#pragma once

#include "XVDRCallbacks.h"
#include <jni.h>

class cJNICallbacks : public cXVDRCallbacks
{
public:

  cJNICallbacks(JavaVM* java);

  virtual ~cJNICallbacks();

  void SetHandle(JNIEnv* env, jobject obj);

  void Log(LEVEL level, const std::string& text, ...);

  void Notification(LEVEL level, const std::string& text, ...);

  void Recording(const std::string& line1, const std::string& line2, bool on);

  void ConvertToUTF8(std::string& text);

  std::string GetLanguageCode();

  std::string GetLocalizedString(int id);

  bool GetSetting(const std::string& setting, void* value);

  void TriggerChannelUpdate();

  void TriggerRecordingUpdate();

  void TriggerTimerUpdate();

  void TransferChannelEntry(PVR_CHANNEL* channel);

  void TransferEpgEntry(EPG_TAG* tag);

  void TransferTimerEntry(PVR_TIMER* timer);

  void TransferRecordingEntry(PVR_RECORDING* rec);

  void TransferChannelGroup(PVR_CHANNEL_GROUP* group);

  void TransferChannelGroupMember(PVR_CHANNEL_GROUP_MEMBER* member);

  XVDRPacket* AllocatePacket(int s);

  uint8_t* GetPacketPayload(XVDRPacket* packet);

  void SetPacketData(XVDRPacket* packet, uint8_t* data, int streamid, uint64_t dts, uint64_t pts);

  void FreePacket(XVDRPacket* packet);

private:

  JNIEnv* m_env;

  jobject m_obj;

  JavaVM* m_java;

};
