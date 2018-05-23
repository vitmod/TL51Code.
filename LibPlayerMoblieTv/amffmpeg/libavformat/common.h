#ifndef FEC_COMMON_H_
#define FEC_COMMON_H_

#include <stdio.h>
#include <stdarg.h>

#include <utils/Log.h>
#include <ctype.h>

#define LOG_TAG "libfec1"
#ifndef LOGV
#define LOGV(...)	LOG_PRI(ANDROID_LOG_VERBOSE, LOG_TAG,__VA_ARGS__)
#endif

#ifndef LOGI
#define LOGI(...)	LOG_PRI(ANDROID_LOG_INFO, LOG_TAG,__VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(...)	LOG_PRI(ANDROID_LOG_WARN, LOG_TAG,__VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...)       LOG_PRI(ANDROID_LOG_ERROR, LOG_TAG,__VA_ARGS__)
#endif

#define TRACE()  LOGV("TARCE:%s:%d\n",__FUNCTION__,__LINE__);


#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define CHECK(condition)                                \
    LOG_ALWAYS_FATAL_IF(                                \
            !(condition),                               \
            "%s",                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
            " CHECK(" #condition ") failed.")



#endif
