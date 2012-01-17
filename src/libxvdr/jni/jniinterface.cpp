#include <jni.h>
#include "jnicallbacks.h"
#include "XVDRData.h"
#include "XVDRThread.h"

#include <android/log.h>
#include <vector>
#include <map>

class cXVDRContext {
public:

    cXVDRContext() : data(NULL) {
	data = NULL;
    };

    ~cXVDRContext() {
	delete data;
    }

    cXVDRData* data;
    cMutex mutex;
};

static std::map<jobject, cXVDRContext*> objmap;
static cJNICallbacks* callbacks;

extern "C" {
    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved);
    JNIEXPORT jboolean JNICALL Java_org_xvdr_XVDRInterface_Connect(JNIEnv* env, jobject jobj, jstring hostname);
    JNIEXPORT void JNICALL Java_org_xvdr_XVDRInterface_Disconnect(JNIEnv* env, jobject jobj);
    JNIEXPORT bool JNICALL Java_org_xvdr_XVDRInterface_GetChannels(JNIEnv* env, jobject jobj, jboolean radio);
}

#define CMD_LOCK(c) cMutexLock lock(&(c->mutex))

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    callbacks = new cJNICallbacks(vm);
    cXVDRCallbacks::Register(callbacks);

    __android_log_print(ANDROID_LOG_INFO, "XVDR", "libxvdrjni: JNI_OnLoad");
    return JNI_VERSION_1_4;
}

JNIEXPORT jboolean JNICALL Java_org_xvdr_XVDRInterface_Connect(JNIEnv* env, jobject jobj, jstring hostname) {
    cXVDRContext* context = objmap[jobj];

    if(context == NULL) {
	context = new cXVDRContext;
	objmap[jobj] = context;
    }

    CMD_LOCK(context);

    callbacks->SetHandle(env, jobj);

    if(context->data == NULL)
    {
    	context->data = new cXVDRData;
    	context->data->SetTimeout(3000);
    }

    const char* host = env->GetStringUTFChars(hostname, NULL);

    if(!context->data->Open(host, "android client device"))
    {
    	env->ReleaseStringUTFChars(hostname, host);
    	return false;
    }

    env->ReleaseStringUTFChars(hostname, host);

    if(!context->data->Login())
    	return false;

    context->data->EnableStatusInterface(true);
    std::vector<int> caids;
    context->data->ChannelFilter(true, false, caids);

    return true;
}

JNIEXPORT void JNICALL Java_org_xvdr_XVDRInterface_Disconnect(JNIEnv* env, jobject jobj) {
    cXVDRContext* context = objmap[jobj];
    CMD_LOCK(context);

    delete context;

    objmap.erase(jobj);
}

JNIEXPORT bool JNICALL Java_org_xvdr_XVDRInterface_GetChannels(JNIEnv* env, jobject jobj, jboolean radio) {
    cXVDRContext* context = objmap[jobj];
    CMD_LOCK(context);

    if(context == NULL) {
	return false;
    }

    context->data->GetChannelsList(radio);
    return true;
}
