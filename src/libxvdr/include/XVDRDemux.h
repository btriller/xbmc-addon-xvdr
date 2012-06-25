#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "XVDRCallbacks.h"
#include "XVDRSession.h"
#include "XVDRThread.h"

#include <string>
#include <queue>

#include "xbmc_pvr_types.h"

class cXVDRResponsePacket;

struct SQuality
{
  std::string fe_name;
  std::string fe_status;
  uint32_t    fe_snr;
  uint32_t    fe_signal;
  uint32_t    fe_ber;
  uint32_t    fe_unc;
};

class cXVDRDemux : public cXVDRSession, protected cThread
{
public:

  cXVDRDemux();
  ~cXVDRDemux();

  bool OpenChannel(const std::string& hostname, const PVR_CHANNEL &channelinfo);
  void Abort();
  bool GetStreamProperties(PVR_STREAM_PROPERTIES* props);
  XVDRPacket* Read();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  int CurrentChannel() { return m_channelinfo.iChannelNumber; }
  bool GetSignalStatus(PVR_SIGNAL_STATUS &qualityinfo);
  void SetPriority(int priority);

protected:

  void Action();

  void OnReconnect();

  XVDRPacket* ReadPacket();

  void CleanupPacketQueue();

  void StreamChange(cXVDRResponsePacket *resp);
  void StreamStatus(cXVDRResponsePacket *resp);
  void StreamSignalInfo(cXVDRResponsePacket *resp);
  bool StreamContentInfo(cXVDRResponsePacket *resp);

private:

  PVR_STREAM_PROPERTIES m_Streams;
  PVR_CHANNEL           m_channelinfo;
  SQuality              m_Quality;
  int                   m_priority;
  std::queue<XVDRPacket*> m_queue;
  cMutex                m_lock;
};
