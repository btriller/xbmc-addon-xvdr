#include "jnicallbacks.h"
#include <android/log.h>

cJNICallbacks::cJNICallbacks(JavaVM* java) : m_env(NULL), m_obj(NULL), m_java(java)
{
}

cJNICallbacks::~cJNICallbacks()
{
}

void cJNICallbacks::SetHandle(JNIEnv* env, jobject obj)
{
  m_env = env;
  m_obj = obj;
}

void cJNICallbacks::Log(LEVEL level, const std::string& text, ...)
{
  int l;
  switch(level)
  {
    case DEBUG:
      l = ANDROID_LOG_DEBUG;
      break;
    default:
    case INFO:
      l = ANDROID_LOG_INFO;
      break;
    case WARNING:
    case NOTICE:
      l = ANDROID_LOG_WARN;
      break;
    case ERROR:
      l = ANDROID_LOG_ERROR;
      break;
  }

  va_list ap;
  va_start(ap, &text);

  __android_log_vprint(l, "XVDR", text.c_str(), ap);

  va_end(ap);
}

void cJNICallbacks::Notification(LEVEL level, const std::string& text, ...)
{
  va_list ap;
  va_start(ap, &text);

  char msg[512];
  vsnprintf(msg, sizeof(msg), text.c_str(), ap);
  va_end(ap);

  JNIEnv* env = NULL;
  if(m_java->AttachCurrentThread(&env, NULL) < 0)
  {
    XVDRLog(XVDR_DEBUG, "AttachCurrentThread - failed!");
    return;
  }

  jclass icls = env->GetObjectClass(m_obj);
  jmethodID jmid = env->GetMethodID(icls, "OnNotification", "(ILjava/lang/String;)V");

  if(jmid == NULL)
    return;

  XVDRLog(XVDR_DEBUG, "%s", __FUNCTION__);

  jobject s = m_env->NewStringUTF(msg);
  env->CallVoidMethod(
           m_obj,
           jmid,
           (int)level,
           s);
  m_env->DeleteLocalRef(s);

  m_java->DetachCurrentThread();

  XVDRLog(XVDR_DEBUG, "%s - Done", __FUNCTION__);
}

void cJNICallbacks::Recording(const std::string& line1, const std::string& line2, bool on)
{
  jclass icls = m_env->GetObjectClass(m_obj);
  jmethodID jmid = m_env->GetMethodID(icls, "OnRecording", "(Ljava/lang/String;Ljava/lang/String;Z)V");

  if(jmid == NULL)
    return;

  m_env->CallVoidMethod(
           m_obj,
           jmid,
           m_env->NewStringUTF(line1.c_str()),
           m_env->NewStringUTF(line2.c_str()),
           on);
}

void cJNICallbacks::ConvertToUTF8(std::string& text)
{
}

std::string cJNICallbacks::GetLanguageCode() {
  return "de";
}

std::string cJNICallbacks::GetLocalizedString(int id)
{
  JNIEnv* env = NULL;
  if(m_java->AttachCurrentThread(&env, NULL) < 0)
  {
    XVDRLog(XVDR_DEBUG, "AttachCurrentThread - failed!");
    return "";
  }

  jclass icls = env->GetObjectClass(m_obj);
  jmethodID jmid = env->GetMethodID(icls, "OnGetLocalizedString", "(I)Ljava/lang/String;");

  if(jmid == NULL)
    return "";

  XVDRLog(XVDR_DEBUG, "%s", __FUNCTION__);

  jstring string = (jstring)env->CallObjectMethod(
           m_obj,
           jmid,
           (int)id);

  const char* s = env->GetStringUTFChars(string, 0);
  std::string result = s;

  env->ReleaseStringUTFChars(string, s);
  m_java->DetachCurrentThread();

  XVDRLog(XVDR_DEBUG, "%s - Done", __FUNCTION__);

  return result;
}

bool cJNICallbacks::GetSetting(const std::string& setting, void* value)
{
  return false;
}

void cJNICallbacks::TriggerChannelUpdate()
{
  JNIEnv* env = NULL;
  if(m_java->AttachCurrentThread(&env, NULL) < 0)
  {
    XVDRLog(XVDR_DEBUG, "AttachCurrentThread - failed!");
    return;
  }

  jclass icls = m_env->GetObjectClass(m_obj);
  jmethodID jmid = m_env->GetMethodID(icls, "OnTriggerChannelUpdate", "()V");

  if(jmid == NULL)
    return;

  m_env->CallVoidMethod(
           m_obj,
           jmid);

  m_java->DetachCurrentThread();
}

void cJNICallbacks::TriggerRecordingUpdate()
{
/*  jclass icls = m_env->GetObjectClass(m_obj);
  jmethodID jmid = m_env->GetMethodID(icls, "OnTriggerRecordingUpdate", "()V");

  if(jmid == NULL)
    return;

  m_env->CallVoidMethod(
           m_obj,
           jmid);*/
}

void cJNICallbacks::TriggerTimerUpdate()
{
/*  jclass jcls = m_env->GetObjectClass(m_obj);
  jmethodID jmid = m_env->GetMethodID(jcls, "OnTriggerTimerUpdate", "()V");

  if(jmid == NULL)
    return;

  m_env->CallVoidMethod(
           m_obj,
           jmid);*/
}

void cJNICallbacks::TransferChannelEntry(PVR_CHANNEL* channel)
{
  jclass jcls = m_env->FindClass("org/xvdr/XVDRChannel");

  if(jcls == NULL)
  {
    Log(ERROR, "JNI: class XVDRChannel not found!");
    return;
  }

  jmethodID jmid = m_env->GetMethodID(jcls, "<init>","()V");

  if(jmid == NULL)
    return;

  jobject jobj = m_env->NewObject(jcls, jmid);

  if(jobj == NULL)
  {
    Log(ERROR, "JNI: Unable to create XVDRChannel object");
    return;
  }

  // set uid
  jfieldID jfid = m_env->GetFieldID(jcls, "uid", "I");
  m_env->SetIntField(jobj, jfid, channel->iUniqueId);

  // set isradio
  jfid = m_env->GetFieldID(jcls, "isradio", "Z");
  m_env->SetBooleanField(jobj, jfid, channel->bIsRadio);

  // set channelnumber
  jfid = m_env->GetFieldID(jcls, "channelnumber", "I");
  m_env->SetIntField(jobj, jfid, channel->iChannelNumber);

  // set channelname
  jfid = m_env->GetFieldID(jcls, "channelname", "Ljava/lang/String;");
  jobject s = m_env->NewStringUTF(channel->strChannelName);
  m_env->SetObjectField(jobj, jfid, s);
  m_env->DeleteLocalRef(s);

  // set caid
  jfid = m_env->GetFieldID(jcls, "caid", "I");
  m_env->SetIntField(jobj, jfid, channel->iEncryptionSystem);

  // set iconurl
  jfid = m_env->GetFieldID(jcls, "iconurl", "Ljava/lang/String;");
  s = m_env->NewStringUTF(channel->strIconPath);
  m_env->SetObjectField(jobj, jfid, s);
  m_env->DeleteLocalRef(s);

  // pass new object back
  jclass jcls_r = m_env->GetObjectClass(m_obj);
  jmid = m_env->GetMethodID(jcls_r, "OnTransferChannelEntry", "(Lorg/xvdr/XVDRChannel;)V");

  if(jmid == NULL)
  {
    Log(ERROR, "JNI: method OnTransferChannelEntry not found!");
    return;
  }

  m_env->CallVoidMethod(
           m_obj,
           jmid,
           jobj);

  m_env->DeleteLocalRef(jobj);
  m_env->DeleteLocalRef(jcls);
  m_env->DeleteLocalRef(jcls_r);
}

void cJNICallbacks::TransferEpgEntry(EPG_TAG* tag)
{
}

void cJNICallbacks::TransferTimerEntry(PVR_TIMER* timer)
{
}

void cJNICallbacks::TransferRecordingEntry(PVR_RECORDING* rec)
{
}

void cJNICallbacks::TransferChannelGroup(PVR_CHANNEL_GROUP* group)
{
}

void cJNICallbacks::TransferChannelGroupMember(PVR_CHANNEL_GROUP_MEMBER* member)
{
}

XVDRPacket* cJNICallbacks::AllocatePacket(int s)
{
  return NULL;
}

uint8_t* cJNICallbacks::GetPacketPayload(XVDRPacket* packet)
{
  return NULL;
}

void cJNICallbacks::SetPacketData(XVDRPacket* packet, uint8_t* data, int streamid, uint64_t dts, uint64_t pts)
{
}

void cJNICallbacks::FreePacket(XVDRPacket* packet)
{
}
