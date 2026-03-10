#include <android/log.h>
#include <android/native_activity.h>

void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize) {
    (void)activity;
    (void)savedState;
    (void)savedStateSize;
    __android_log_print(ANDROID_LOG_INFO, "raymob", "Native stub loaded");
}
