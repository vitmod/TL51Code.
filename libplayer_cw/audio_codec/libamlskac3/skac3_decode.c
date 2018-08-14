
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include "skac3.h"
#include "../../amadec/adec-armdec-mgt.h"
#include "../../amadec/audio-dec.h"
#include <android/log.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif  

#define  LOG_TAG    "Aml_SkAC3_dec"

#define  LOG(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static int g_fdLibEac3 = -1;
void *g_handlerSkEac3 = NULL;
static int (*sk_eac3_init)(void **phandler, int sample_rate, int channels);
static int (*sk_eac3_decode)(void *handler, char *outbuf, int *outlen, char *inbuf, int inlen);
static int (*sk_eac3_release)(void* handler);
static int (*sk_eac3_getinfo)(void *handler , int *channels, int *sample_rate);


static void clear_skeac3_lib() {
	if(g_fdLibEac3 > 0) {
		dlclose(g_fdLibEac3);
		g_fdLibEac3 = -1;
	}
	
	g_fdLibEac3 = 0;
	g_handlerSkEac3 = NULL;
	sk_eac3_init = NULL;
	sk_eac3_decode = NULL;
	sk_eac3_release = NULL;
	sk_eac3_getinfo = NULL;

}


int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
    aml_audio_dec_t *audec = (aml_audio_dec_t *)(adec_ops->priv_data);
	
	//LOG("[%s], inbuf: 0x%x, inlen: %d\n", __FUNCTION__, inbuf, inlen);
	if(NULL != g_handlerSkEac3 && NULL != sk_eac3_decode) {
		return sk_eac3_decode(g_handlerSkEac3, outbuf, outlen, inbuf, inlen);
	}
	
    return -1;

}
int audio_dec_init(audio_decoder_operations_t *adec_ops)
{
    aml_audio_dec_t *audec = (aml_audio_dec_t *)(adec_ops->priv_data);
	
    LOG("\n\n[%s]BuildDate--%s  BuildTime--%s\n", __FUNCTION__, __DATE__, __TIME__);
        
    LOG("[%s]audec->format/%d adec_ops->samplerate/%d adec_ops->channels/%d, audec->data_width/%d, audec->block_align/%d, audec->codec_id/0x%x, nInBufSize/%d, nOutBufZie/%d\n",
           __FUNCTION__, audec->format, adec_ops->samplerate, adec_ops->channels,
		   audec->data_width, audec->block_align, audec->codec_id,
		   adec_ops->nInBufSize, adec_ops->nOutBufSize);
	
	clear_skeac3_lib();
	
	g_fdLibEac3 = dlopen("libskeac3decode.so", RTLD_NOW);
    if (g_fdLibEac3 != 0) {
        sk_eac3_init    = dlsym(g_fdLibEac3, "skac3_audio_dec_init");
        sk_eac3_decode  = dlsym(g_fdLibEac3, "skac3_audio_dec_decode");
        sk_eac3_release = dlsym(g_fdLibEac3, "skac3_audio_dec_release");
        sk_eac3_getinfo = dlsym(g_fdLibEac3, "skac3_audio_dec_getinfo");
		
		if(NULL == sk_eac3_init || 
			NULL == sk_eac3_decode ||
			NULL == sk_eac3_release ||
			NULL == sk_eac3_getinfo) {
			LOG("No symc in lib\n");
			clear_skeac3_lib();
			return -1;
		} else {
			LOG("loaded skyworth skeac3 decoder lib\n");
		}
    } else {
		LOG("[%s] Filed to load libskeac3decode.so %d %s\n", __FUNCTION__, g_fdLibEac3, dlerror());
		clear_skeac3_lib();
		return -1;
	}
	
	
	if(0 != sk_eac3_init(&g_handlerSkEac3, adec_ops->samplerate, adec_ops->channels)) {
		clear_skeac3_lib();
		LOG("[%s] sk_eac3_init error\n", __FUNCTION__);
		return -1;
	}
	
	adec_ops->nInBufSize = 4096;
		
    return 0;
}

int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
    LOG("\n\n[%s]BuildDate--%s  BuildTime--%s\n", __FUNCTION__, __DATE__, __TIME__);
	if(NULL != g_handlerSkEac3 && NULL != sk_eac3_release) {
		sk_eac3_release(g_handlerSkEac3);
	}
	
	clear_skeac3_lib();
	
    return 0;
}

int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
	int channels = 2;
	int sample_rate = 48000;
	//LOG("[%s], pAudioInfo: 0x%x\n",__FUNCTION__, pAudioInfo);
	if(NULL != g_handlerSkEac3 && NULL != sk_eac3_getinfo) {
		sk_eac3_getinfo(g_handlerSkEac3, &channels, &sample_rate);
	}
	adec_ops->NchOriginal = channels;
	((AudioInfo*)pAudioInfo)->channels = channels;
	((AudioInfo*)pAudioInfo)->samplerate = sample_rate;

    return 0;
}


