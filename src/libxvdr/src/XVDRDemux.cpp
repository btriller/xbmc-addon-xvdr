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

#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "codecids.h" // For codec id's
#include "XVDRDemux.h"
#include "XVDRCallbacks.h"
#include "XVDRResponsePacket.h"
#include "requestpacket.h"
#include "xvdrcommand.h"

#define DMX_SPECIALID_STREAMINFO    -10
#define DMX_SPECIALID_STREAMCHANGE  -11

cXVDRDemux::cXVDRDemux() : m_priority(50)
{
  m_Streams.iStreamCount = 0;
}

cXVDRDemux::~cXVDRDemux()
{
}

bool cXVDRDemux::OpenChannel(const std::string& hostname, const PVR_CHANNEL &channelinfo)
{
  m_channelinfo = channelinfo;
  if(!cXVDRSession::Open(hostname))
    return false;

  if(!cXVDRSession::Login())
    return false;

  return SwitchChannel(m_channelinfo);
}

bool cXVDRDemux::GetStreamProperties(PVR_STREAM_PROPERTIES* props)
{
  props->iStreamCount = m_Streams.iStreamCount;
  for (unsigned int i = 0; i < m_Streams.iStreamCount; i++)
  {
    props->stream[i].iStreamIndex = m_Streams.stream[i].iStreamIndex;
    props->stream[i].iPhysicalId  = m_Streams.stream[i].iPhysicalId;
    props->stream[i].iCodecType   = m_Streams.stream[i].iCodecType;
    props->stream[i].iCodecId     = m_Streams.stream[i].iCodecId;
    props->stream[i].iHeight      = m_Streams.stream[i].iHeight;
    props->stream[i].iWidth       = m_Streams.stream[i].iWidth;
    props->stream[i].iIdentifier  = m_Streams.stream[i].iIdentifier;

    memcpy(props->stream[i].strLanguage, m_Streams.stream[i].strLanguage, 4);
  }
  return (props->iStreamCount > 0);
}

void cXVDRDemux::Abort()
{
  m_Streams.iStreamCount = 0;
  cXVDRSession::Abort();
}

XVDRPacket* cXVDRDemux::Read()
{
  if(ConnectionLost())
  {
    SleepMs(100);
    return XVDRAllocatePacket(0);
  }

  cXVDRResponsePacket *resp = ReadMessage();

  if(resp == NULL)
    return XVDRAllocatePacket(0);

  if (resp->getChannelID() != XVDR_CHANNEL_STREAM)
  {
    delete resp;
    return XVDRAllocatePacket(0);
  }

  XVDRPacket* pkt = NULL;
  int iStreamId = -1;

  switch (resp->getOpCodeID())
  {
    case XVDR_STREAM_CHANGE:
      StreamChange(resp);
      pkt = XVDRAllocatePacket(0);
      if (pkt != NULL)
        XVDRSetPacketData(pkt, NULL, DMX_SPECIALID_STREAMCHANGE, 0, 0);

      delete resp;
      return pkt;

    case XVDR_STREAM_STATUS:
      StreamStatus(resp);
      break;

    case XVDR_STREAM_SIGNALINFO:
      StreamSignalInfo(resp);
      break;

    case XVDR_STREAM_CONTENTINFO:
      // send stream updates only if there are changes
      if(!StreamContentInfo(resp))
        break;

      pkt = XVDRAllocatePacket(sizeof(PVR_STREAM_PROPERTIES));
      if (pkt != NULL)
        XVDRSetPacketData(pkt, (uint8_t*)&m_Streams, DMX_SPECIALID_STREAMINFO, 0, 0);

      delete resp;
      return pkt;

    case XVDR_STREAM_MUXPKT:
      // figure out the stream id for this packet
      for(unsigned int i = 0; i < m_Streams.iStreamCount; i++)
      {
        if(m_Streams.stream[i].iPhysicalId == (unsigned int)resp->getStreamID())
        {
          iStreamId = i;
          break;
        }
      }

      // stream found ?
      if(iStreamId != -1)
      {
        pkt = (XVDRPacket*)resp->getUserData();
        if (pkt != NULL)
          XVDRSetPacketData(pkt, NULL, iStreamId, resp->getDTS(), resp->getPTS());

        delete resp;
        return pkt;
      }
      else
      {
        XVDRLog(XVDR_DEBUG, "stream id %i not found", resp->getStreamID());
      }
      break;
  }

  delete resp;
  return XVDRAllocatePacket(0);
}

bool cXVDRDemux::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XVDRLog(XVDR_DEBUG, "changing to channel %d (priority %i)", channelinfo.iChannelNumber, m_priority);

  cRequestPacket vrp;
  uint32_t rc = 0;

  if (vrp.init(XVDR_CHANNELSTREAM_OPEN) && vrp.add_U32(channelinfo.iUniqueId) && vrp.add_S32(m_priority) && ReadSuccess(&vrp, rc))
  {
    m_channelinfo = channelinfo;
    m_Streams.iStreamCount  = 0;

    return true;
  }

  switch (rc)
  {
    // active recording
    case XVDR_RET_RECRUNNING:
     XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30062));
      break;
    // all receivers busy
    case XVDR_RET_DATALOCKED:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30063));
      break;
    // encrypted channel
    case XVDR_RET_ENCRYPTED:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30066));
      break;
    // error on switching channel
    default:
    case XVDR_RET_ERROR:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30064));
      break;
    // invalid channel
    case XVDR_RET_DATAINVALID:
      XVDRNotification(XVDR_ERROR, XVDRGetLocalizedString(30065), channelinfo.strChannelName);
      break;
  }

  XVDRLog(XVDR_ERROR, "%s - failed to set channel", __FUNCTION__);
  return true;
}

bool cXVDRDemux::GetSignalStatus(PVR_SIGNAL_STATUS &qualityinfo)
{
  if (ConnectionLost())
    return false;

  if (m_Quality.fe_name.empty())
  {
    memset(&qualityinfo, 0, sizeof(PVR_SIGNAL_STATUS));
    return true;
  }

  strncpy(qualityinfo.strAdapterName, m_Quality.fe_name.c_str(), sizeof(qualityinfo.strAdapterName));
  strncpy(qualityinfo.strAdapterStatus, m_Quality.fe_status.c_str(), sizeof(qualityinfo.strAdapterStatus));
  qualityinfo.iSignal = (uint16_t)m_Quality.fe_signal;
  qualityinfo.iSNR = (uint16_t)m_Quality.fe_snr;
  qualityinfo.iBER = (uint32_t)m_Quality.fe_ber;
  qualityinfo.iUNC = (uint32_t)m_Quality.fe_unc;
  qualityinfo.dVideoBitrate = 0;
  qualityinfo.dAudioBitrate = 0;
  qualityinfo.dDolbyBitrate = 0;

  return true;
}

void cXVDRDemux::StreamChange(cXVDRResponsePacket *resp)
{
  m_Streams.iStreamCount = 0;

  while (!resp->end())
  {
    uint32_t    id   = resp->extract_U32();
    const char* type = resp->extract_String();

    struct PVR_STREAM_PROPERTIES::PVR_STREAM* stream = &m_Streams.stream[m_Streams.iStreamCount];

    stream->iFPSScale      = 0;
    stream->iFPSRate       = 0;
    stream->iHeight        = 0;
    stream->iWidth         = 0;
    stream->fAspect        = 0.0;

    stream->iChannels      = 0;
    stream->iSampleRate    = 0;
    stream->iBlockAlign    = 0;
    stream->iBitRate       = 0;
    stream->iBitsPerSample = 0;

    stream->iStreamIndex   = m_Streams.iStreamCount;
    stream->iPhysicalId    = id;
    stream->iIdentifier    = -1;

    memset(stream->strLanguage, 0, 4);

    if(!strcmp(type, "AC3"))
    {
      const char *language = resp->extract_String();

      stream->iCodecType     = AVMEDIA_TYPE_AUDIO;
      stream->iCodecId       = CODEC_ID_AC3;

      memcpy(stream->strLanguage, language, 3);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      const char *language = resp->extract_String();

      stream->iCodecType     = AVMEDIA_TYPE_AUDIO;
      stream->iCodecId       = CODEC_ID_MP2;

      memcpy(stream->strLanguage, language, 3);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "AAC"))
    {
      const char *language = resp->extract_String();

      stream->iCodecType     = AVMEDIA_TYPE_AUDIO;
      stream->iCodecId       = CODEC_ID_AAC;

      memcpy(stream->strLanguage, language, 3);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "LATM"))
    {
      const char *language = resp->extract_String();

      stream->iCodecType     = AVMEDIA_TYPE_AUDIO;
      stream->iCodecId       = CODEC_ID_AAC_LATM;

      memcpy(stream->strLanguage, language, 3);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "DTS"))
    {
      const char *language = resp->extract_String();

      stream->iCodecType     = AVMEDIA_TYPE_AUDIO;
      stream->iCodecId       = CODEC_ID_DTS;

      memcpy(stream->strLanguage, language, 3);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "EAC3"))
    {
      const char *language = resp->extract_String();

      stream->iCodecType     = AVMEDIA_TYPE_AUDIO;
      stream->iCodecId       = CODEC_ID_EAC3;

      memcpy(stream->strLanguage, language, 3);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      stream->iCodecType     = AVMEDIA_TYPE_VIDEO;
      stream->iCodecId       = CODEC_ID_MPEG2VIDEO;
      stream->iFPSScale      = resp->extract_U32();
      stream->iFPSRate       = resp->extract_U32();
      stream->iHeight        = resp->extract_U32();
      stream->iWidth         = resp->extract_U32();
      stream->fAspect        = resp->extract_Double();

      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "H264"))
    {
      stream->iCodecType     = AVMEDIA_TYPE_VIDEO;
      stream->iCodecId       = CODEC_ID_H264;
      stream->iFPSScale      = resp->extract_U32();
      stream->iFPSRate       = resp->extract_U32();
      stream->iHeight        = resp->extract_U32();
      stream->iWidth         = resp->extract_U32();
      stream->fAspect        = resp->extract_Double();

      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "DVBSUB"))
    {
      const char *language    = resp->extract_String();
      uint32_t composition_id = resp->extract_U32();
      uint32_t ancillary_id   = resp->extract_U32();

      stream->iCodecType  = AVMEDIA_TYPE_SUBTITLE;
      stream->iCodecId    = CODEC_ID_DVB_SUBTITLE;
      stream->iIdentifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);

      memcpy(stream->strLanguage, language, 3);
      m_Streams.iStreamCount++;
    }
    else if(!strcmp(type, "TELETEXT"))
    {
      stream->iCodecType = AVMEDIA_TYPE_SUBTITLE;
      stream->iCodecId   = CODEC_ID_DVB_TELETEXT;

      m_Streams.iStreamCount++;
    }

    if (m_Streams.iStreamCount >= PVR_STREAM_MAX_STREAMS)
    {
      XVDRLog(XVDR_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }
  }
}

void cXVDRDemux::StreamStatus(cXVDRResponsePacket *resp)
{
  uint32_t status = resp->extract_U32();

  switch(status) {
    case XVDR_STREAM_STATUS_SIGNALLOST:
      XVDRNotification(XVDR_ERROR, XVDRGetLocalizedString(30047));
      break;
    case XVDR_STREAM_STATUS_SIGNALRESTORED:
      XVDRNotification(XVDR_INFO, XVDRGetLocalizedString(30048));
      SwitchChannel(m_channelinfo);
      break;
    default:
      break;
  }
}

void cXVDRDemux::StreamSignalInfo(cXVDRResponsePacket *resp)
{
  const char* name = resp->extract_String();
  const char* status = resp->extract_String();

  m_Quality.fe_name   = name;
  m_Quality.fe_status = status;
  m_Quality.fe_snr    = resp->extract_U32();
  m_Quality.fe_signal = resp->extract_U32();
  m_Quality.fe_ber    = resp->extract_U32();
  m_Quality.fe_unc    = resp->extract_U32();
}

bool cXVDRDemux::StreamContentInfo(cXVDRResponsePacket *resp)
{
  PVR_STREAM_PROPERTIES old = m_Streams;

  for (unsigned int i = 0; i < m_Streams.iStreamCount && !resp->end(); i++)
  {
    uint32_t id = resp->extract_U32();
    struct PVR_STREAM_PROPERTIES::PVR_STREAM* stream = NULL;

    // find stream
    for (unsigned int j = 0; j < m_Streams.iStreamCount; j++)
    {
   	   stream = &m_Streams.stream[j];
       if (m_Streams.stream[j].iPhysicalId == id)
    	 break;
    }

    if (stream == NULL)
      continue;

    const char* language    = NULL;
    uint32_t composition_id = 0;
    uint32_t ancillary_id   = 0;

    switch (stream->iCodecType)
    {
      case AVMEDIA_TYPE_AUDIO:
        language = resp->extract_String();

        stream->iChannels      = resp->extract_U32();
        stream->iSampleRate    = resp->extract_U32();
        stream->iBlockAlign    = resp->extract_U32();
        stream->iBitRate       = resp->extract_U32();
        stream->iBitsPerSample = resp->extract_U32();
        stream->strLanguage[3] = 0;
        memcpy(stream->strLanguage, language, 3);
        break;

      case AVMEDIA_TYPE_VIDEO:
        stream->iFPSScale = resp->extract_U32();
        stream->iFPSRate  = resp->extract_U32();
        stream->iHeight   = resp->extract_U32();
        stream->iWidth    = resp->extract_U32();
        stream->fAspect   = resp->extract_Double();
        break;

      case AVMEDIA_TYPE_SUBTITLE:
        language = resp->extract_String();

        composition_id = resp->extract_U32();
        ancillary_id   = resp->extract_U32();

        stream->iIdentifier    = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
        stream->strLanguage[3] = 0;
        memcpy(stream->strLanguage, language, 3);
        break;

      default:
        break;
    }
  }

  return (memcmp(&old, &m_Streams, sizeof(m_Streams)) != 0);
}

void cXVDRDemux::OnReconnect()
{
}

void cXVDRDemux::SetPriority(int priority)
{
  if(priority < -1 || priority > 99)
    priority = 50;

  m_priority = priority;
}
