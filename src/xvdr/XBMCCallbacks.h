#pragma once

#include "XBMCAddon.h"
#include "xvdr/callbacks.h"
#include "xvdr/thread.h"

#include "xbmc_pvr_types.h"

class cXBMCCallbacks : public XVDR::Callbacks
{
public:

  cXBMCCallbacks();

  void Log(LEVEL level, const std::string& text, ...);

  void Notification(LEVEL level, const std::string& text, ...);

  void Recording(const std::string& line1, const std::string& line2, bool on);

  void ConvertToUTF8(std::string& text);

  std::string GetLanguageCode();

  const char* GetLocalizedString(int id);

  void TriggerChannelUpdate();

  void TriggerRecordingUpdate();

  void TriggerTimerUpdate();

  void SetHandle(ADDON_HANDLE handle);

  void TransferChannelEntry(const XVDR::Channel& channel);

  void TransferEpgEntry(const XVDR::Epg& tag);

  void TransferTimerEntry(const XVDR::Timer& timer);

  void TransferRecordingEntry(const XVDR::RecordingEntry& rec);

  void TransferChannelGroup(const XVDR::ChannelGroup& group);

  void TransferChannelGroupMember(const XVDR::ChannelGroupMember& member);

  XVDR::Packet* AllocatePacket(int length);

  uint8_t* GetPacketPayload(XVDR::Packet* packet);

  void SetPacketData(XVDR::Packet* packet, uint8_t* data = NULL, int streamid = 0, uint64_t dts = 0, uint64_t pts = 0);

  void FreePacket(XVDR::Packet* packet);

  XVDR::Packet* StreamChange(const XVDR::StreamProperties& p);

  XVDR::Packet* ContentInfo(const XVDR::StreamProperties& p);

private:

  ADDON_HANDLE m_handle;
};

PVR_CHANNEL& operator<< (PVR_CHANNEL& lhs, const XVDR::Channel& rhs);

EPG_TAG& operator<< (EPG_TAG& lhs, const XVDR::Epg& rhs);

XVDR::Timer& operator<< (XVDR::Timer& lhs, const PVR_TIMER& rhs);

PVR_TIMER& operator<< (PVR_TIMER& lhs, const XVDR::Timer& rhs);

XVDR::RecordingEntry& operator<< (XVDR::RecordingEntry& lhs, const PVR_RECORDING& rhs);

PVR_RECORDING& operator<< (PVR_RECORDING& lhs, const XVDR::RecordingEntry& rhs);

PVR_CHANNEL_GROUP& operator<< (PVR_CHANNEL_GROUP& lhs, const XVDR::ChannelGroup& rhs);

PVR_CHANNEL_GROUP_MEMBER& operator<< (PVR_CHANNEL_GROUP_MEMBER& lhs, const XVDR::ChannelGroupMember& rhs);

PVR_STREAM_PROPERTIES& operator<< (PVR_STREAM_PROPERTIES& lhs, const XVDR::StreamProperties& rhs);

PVR_STREAM_PROPERTIES::PVR_STREAM& operator<< (PVR_STREAM_PROPERTIES::PVR_STREAM& lhs, const XVDR::Stream& rhs);

PVR_SIGNAL_STATUS& operator<< (PVR_SIGNAL_STATUS& lhs, const XVDR::SignalStatus& rhs);
