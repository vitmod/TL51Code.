
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include "skdts.h"
#include "../../amadec/adec-armdec-mgt.h"
#include "../../amadec/audio-dec.h"
#include <android/log.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif  

#define  LOG_TAG    "Aml_SkDTS_dec"

#define  LOG(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static int g_fdLibDts = -1;
void *g_handlerSkDts = NULL;
static int (*sk_dts_init)(void **phandler, int sample_rate, int channels);
static int (*sk_dts_decode)(void *handler, char *outbuf, int *outlen, char *inbuf, int inlen);
static int (*sk_dts_release)(void* handler);
static int (*sk_dts_getinfo)(void *handler , int *channels, int *sample_rate);


static void clear_skdts_lib() {
	if(g_fdLibDts > 0) {
		dlclose(g_fdLibDts);
		g_fdLibDts = -1;
	}
	
	g_fdLibDts = 0;
	g_handlerSkDts = NULL;
	sk_dts_init = NULL;
	sk_dts_decode = NULL;
	sk_dts_release = NULL;
	sk_dts_getinfo = NULL;

}


int audio_dec_decode(audio_decoder_operations_t *adec_ops, char *outbuf, int *outlen, char *inbuf, int inlen)
{
    aml_audio_dec_t *audec = (aml_audio_dec_t *)(adec_ops->priv_data);
	
	//LOG("[%s], inbuf: 0x%x, inlen: %d\n", __FUNCTION__, inbuf, inlen);
	if(NULL != g_handlerSkDts && NULL != sk_dts_decode) {
		return sk_dts_decode(g_handlerSkDts, outbuf, outlen, inbuf, inlen);
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
	
	clear_skdts_lib();
	
	g_fdLibDts = dlopen("libskdtsdecode.so", RTLD_NOW);
    if (g_fdLibDts != 0) {
        sk_dts_init    = dlsym(g_fdLibDts, "skdts_audio_dec_init");
        sk_dts_decode  = dlsym(g_fdLibDts, "skdts_audio_dec_decode");
        sk_dts_release = dlsym(g_fdLibDts, "skdts_audio_dec_release");
        sk_dts_getinfo = dlsym(g_fdLibDts, "skdts_audio_dec_getinfo");
		
		if(NULL == sk_dts_init || 
			NULL == sk_dts_decode ||
			NULL == sk_dts_release ||
			NULL == sk_dts_getinfo) {
			LOG("No symc in lib\n");
			clear_skdts_lib();
			return -1;
		} else {
			LOG("loaded skyworth skdts decoder lib\n");
		}
    } else {
		LOG("[%s] Filed to load libskdtsdecode.so %d %s\n", __FUNCTION__, g_fdLibDts, dlerror());
		clear_skdts_lib();
		return -1;
	}
	
	
	if(0 != sk_dts_init(&g_handlerSkDts, adec_ops->samplerate, adec_ops->channels)) {
		clear_skdts_lib();
		LOG("[%s] sk_dts_init error\n", __FUNCTION__);
		return -1;
	}
	
	adec_ops->nInBufSize = 4096;
		
    return 0;
}

int audio_dec_release(audio_decoder_operations_t *adec_ops)
{
    LOG("\n\n[%s]BuildDate--%s  BuildTime--%s\n", __FUNCTION__, __DATE__, __TIME__);
	if(NULL != g_handlerSkDts && NULL != sk_dts_release) {
		sk_dts_release(g_handlerSkDts);
	}
	
	clear_skdts_lib();
	
    return 0;
}

int audio_dec_getinfo(audio_decoder_operations_t *adec_ops, void *pAudioInfo)
{
	int channels = 2;
	int sample_rate = 48000;
	//LOG("[%s], pAudioInfo: 0x%x\n",__FUNCTION__, pAudioInfo);
	if(NULL != g_handlerSkDts && NULL != sk_dts_getinfo) {
		sk_dts_getinfo(g_handlerSkDts, &channels, &sample_rate);
	}
	adec_ops->NchOriginal = channels;
	((AudioInfo*)pAudioInfo)->channels = channels;
	((AudioInfo*)pAudioInfo)->samplerate = sample_rate;

    return 0;
}


