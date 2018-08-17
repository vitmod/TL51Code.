/************************************************
 * name : player_para.c
 * function: ffmpeg file relative and set player parameters functions
 * date     : 2010.2.4
 ************************************************/
#include <codec.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <player_set_sys.h>

#include "thread_mgt.h"
#include "stream_decoder.h"
#include "player_priv.h"
#include "player_hwdec.h"
#include "player_update.h"
#include "player_ffmpeg_ctrl.h"
#include "system/systemsetting.h"
#include <cutils/properties.h>
#include <iconv.h>

#define DTS_ENABLE_PATH "/sys/class/amaudio/dts_enable"
#define DOBLY_ENABLE_PATH "/sys/class/amaudio/dolby_enable"

extern es_sub_t es_sub_buf[SSTREAM_MAX_NUM];

DECLARE_ALIGNED(16, uint8_t, dec_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2]);

static int try_decode_picture(play_para_t *p_para, int video_index)
{
    AVCodecContext *ic = NULL;
    AVCodec *codec = NULL;
    AVFrame *picture = NULL;
    int got_picture = 0;
    int ret = 0;
    int read_packets = 0;
    int64_t cur_pos;
    AVPacket avpkt;
    int try_readframe_count = 0;
    int try_readframe_count_max = am_getconfig_int_def("media.amplayer.try_decode",280);
    ic = p_para->pFormatCtx->streams[video_index]->codec;

    codec = avcodec_find_decoder(ic->codec_id);
    if (!codec) {
        log_print("[%s:%d]Codec not found\n", __FUNCTION__, __LINE__);
        goto exitf1;
    }

    if (avcodec_open(ic, codec) < 0) {
        log_print("[%s:%d]Could not open codec\n", __FUNCTION__, __LINE__);
        goto exitf1;
    }

    picture = avcodec_alloc_frame();
    if (!picture) {
        log_print("[%s:%d]Could not allocate picture\n", __FUNCTION__, __LINE__);
        goto exitf;
    }

    cur_pos = url_ftell(p_para->pFormatCtx->pb);
    log_print("[%s:%d]codec id 0x%x, cur_pos 0x%llx, video index %d\n",
              __FUNCTION__, __LINE__, ic->codec_id, cur_pos, video_index);
    av_init_packet(&avpkt);

    /* get the first video frame and decode it */
    while (!got_picture) {
        do {
            ret = av_read_frame(p_para->pFormatCtx, &avpkt);
            if (ret < 0) {
                if (AVERROR(EAGAIN) != ret) {
                    /*if the return is EAGAIN,we need to try more times*/
                    log_error("[%s:%d]av_read_frame return (%d)\n", __FUNCTION__, __LINE__, ret);
                    url_fseek(p_para->pFormatCtx->pb, cur_pos, SEEK_SET);
                    av_free_packet(&avpkt);
                    goto exitf;
                } else {
                    av_free_packet(&avpkt);
                    continue;
                }
            }
        } while ((avpkt.stream_index != video_index) && (++try_readframe_count <= try_readframe_count_max));

        if (try_readframe_count > try_readframe_count_max) {
            url_fseek(p_para->pFormatCtx->pb, cur_pos, SEEK_SET);
            av_free_packet(&avpkt);
            log_error("[%s:%d]av_read_frame index %d more than %d times, return\n", __FUNCTION__, __LINE__, video_index, try_readframe_count);
            return -1;
        }

        avcodec_decode_video2(ic, picture, &got_picture, &avpkt);
        av_free_packet(&avpkt);
        read_packets++;
    }

    log_print("[%s:%d]got one picture, try_readframe_count:%d\n", __FUNCTION__, __LINE__, try_readframe_count);
    if (picture) {
        av_free(picture);
    }

    url_fseek(p_para->pFormatCtx->pb, cur_pos, SEEK_SET);
    avcodec_close(ic);
    return 0;

exitf:
    if (picture) {
        av_free(picture);
    }
    avcodec_close(ic);
exitf1:

    if (read_packets) {
        return read_packets;
    } else {
        return ret;
    }
}

static int check_codec_parameters_ex(AVCodecContext *enc, int fastmode)
{
    int val;
    if (!fastmode) {
        switch (enc->codec_type) {
        case AVMEDIA_TYPE_AUDIO:

            val = enc->sample_rate && enc->channels;
            if (!enc->frame_size &&
                (enc->codec_id == CODEC_ID_VORBIS ||
                 enc->codec_id == CODEC_ID_AAC ||
                 enc->codec_id == CODEC_ID_MP1 ||
                 enc->codec_id == CODEC_ID_MP2 ||
                 enc->codec_id == CODEC_ID_MP3 ||
                 enc->codec_id == CODEC_ID_SPEEX ||
                 enc->codec_id == CODEC_ID_CELT)) {
                return 0;
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            val = enc->width && enc->pix_fmt != PIX_FMT_NONE;
            break;
        default:
            val = 1;
            break;
        }
    } else {
        switch (enc->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            if (fastmode == 2) { //maybe ts audio need set fastmode=2
                val = 1;
            } else {
                val = enc->sample_rate && enc->channels;
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            val = enc->width;
            break;
        default:
            val = 1;
            break;
        }
    }
    return enc->codec_id != CODEC_ID_NONE && val != 0;
}
static void get_audio_type(play_para_t *p_para)
{
    AVFormatContext *pFormatCtx = p_para->pFormatCtx;
    AVStream *pStream;
    AVCodecContext  *pCodecCtx;
    int audio_index = p_para->astream_info.audio_index;
    if (audio_index != -1) {
            pStream = pFormatCtx->streams[audio_index];
            pCodecCtx = pStream->codec;
            p_para->astream_info.audio_pid      = (unsigned short)pStream->id;
            p_para->astream_info.audio_format   = audio_type_convert(pCodecCtx->codec_id, p_para->file_type, 0);
            if (pFormatCtx->drmcontent) {
                log_print("[%s:%d]DRM content found, not support yet.\n", __FUNCTION__, __LINE__);
                p_para->astream_info.audio_format = AFORMAT_UNSUPPORT;
            }
            p_para->astream_info.audio_channel  = pCodecCtx->channels;
            p_para->astream_info.audio_samplerate = pCodecCtx->sample_rate;
            log_print("[%s:%d]afmt=%d apid=%d asr=%d ach=%d aidx=%d\n",
                      __FUNCTION__, __LINE__, p_para->astream_info.audio_format,
                      p_para->astream_info.audio_pid, p_para->astream_info.audio_samplerate,
                      p_para->astream_info.audio_channel, p_para->astream_info.audio_index);
            /* only support 2ch flac,cook,raac */
            if ((p_para->astream_info.audio_channel > 2) &&
                (IS_AUDIO_NOT_SUPPORT_EXCEED_2CH(p_para->astream_info.audio_format))) {
                log_print(" afmt=%d channel=%d ******** we do not support more than 2ch \n", \
                          p_para->astream_info.audio_format, p_para->astream_info.audio_channel);
                p_para->astream_info.has_audio = 0;
            }
    
            //----------------------------------
            //more than 6 ch was not support
            if ((p_para->astream_info.audio_channel > 6) &&
                (IS_AUDIO_NOT_SUPPORT_EXCEED_6CH(p_para->astream_info.audio_format))) {
                log_print(" afmt=%d channel=%d ******** we do not support more than 6ch \n", \
                          p_para->astream_info.audio_format, p_para->astream_info.audio_channel);
                p_para->astream_info.has_audio = 0;
            }
    
            //more than 48000 was not support
            if ((p_para->astream_info.audio_samplerate > 48000) &&
                (IS_AUDIO_NOT_SUPPORT_EXCEED_FS48k(p_para->astream_info.audio_format))) {
                log_print(" afmt=%d sample_rate=%d ******** we do not support more than 48000 \n", \
                          p_para->astream_info.audio_format, p_para->astream_info.audio_samplerate);
                p_para->astream_info.has_audio = 0;
            }
            //ape audio 16 bps support
            if ((p_para->astream_info.audio_format == AFORMAT_APE) &&
                pCodecCtx->bits_per_coded_sample != 16) {
                log_print(" ape audio only support 16 bit  bps \n");
                p_para->astream_info.has_audio = 0;
            }
            //---------------------------------
            if (p_para->astream_info.has_audio == 1 &&
                p_para->vstream_info.has_video == 0 &&
                (p_para->astream_info.audio_format == AFORMAT_COOK ||
                 p_para->astream_info.audio_format == AFORMAT_SIPR)
               ) {
                log_print("[%s %d]RM Pure Audio Stream,COVERT p_para->stream_type to STREAM_ES\n", __FUNCTION__, __LINE__);
                p_para->stream_type = STREAM_ES;
                p_para->playctrl_info.raw_mode = 0;
                //p_para->file_type=STREAM_FILE;
            }
            if (p_para->astream_info.has_audio == 1 &&
                p_para->vstream_info.has_video == 0 &&
                p_para->file_type == RM_FILE && p_para->astream_info.audio_format == AFORMAT_AC3) {
                log_print("[%s %d]RM Pure Audio Stream,COVERT p_para->stream_type to STREAM_ES\n", __FUNCTION__, __LINE__);
                p_para->stream_type = STREAM_ES;
                p_para->playctrl_info.raw_mode = 0;
            }
    
            if (p_para->astream_info.audio_format == AFORMAT_AAC || p_para->astream_info.audio_format == AFORMAT_AAC_LATM) {
                pCodecCtx->profile = FF_PROFILE_UNKNOWN;
                AVCodecContext  *pCodecCtx = p_para->pFormatCtx->streams[audio_index]->codec;
                uint8_t *ppp = pCodecCtx->extradata;
    
                if (ppp != NULL) {
                    char profile;
                    if ((ppp[0] == 0xFF) && ((ppp[1] & 0xF0) == 0xF0)) {
                        profile = (ppp[2] >> 6) + 1;
                    } else {
                        profile = (*ppp) >> 3;
                    }
                    log_print(" aac profile = %d  ********* { MAIN, LC, SSR } \n", profile);
    
                    if (profile == 1) {
                        pCodecCtx->profile = FF_PROFILE_AAC_MAIN;
                        log_print("AAC MAIN only  support by arm audio decoder,will do the auto switch to arm decoder\n");
    
#if 0
                        /*add main profile support if choose arm audio decoder*/
                        char value[PROPERTY_VALUE_MAX];
                        int ret = property_get("media.arm.audio.decoder", value, NULL);
                        if (ret > 0 && match_types("aac", value)) {
                            log_print("AAC MAIN support by arm audio decoder!!\n");
                        } else {
                            p_para->astream_info.has_audio = 0;
                            log_print("AAC MAIN not support yet!!\n");
                        }
#endif
                    }
                    //else
                    //  p_para->astream_info.has_audio = 0;
                }
                /* First packet would lose if enable this code when mpegts goes through cmf. By senbai.tao, 2012.12.14 */
#if 0
                else {
    
                    AVCodec * aac_codec;
                    if (pCodecCtx->codec_id == CODEC_ID_AAC_LATM) {
                        aac_codec = avcodec_find_decoder_by_name("aac_latm");
                    } else {
                        aac_codec = avcodec_find_decoder_by_name("aac");
                    }
    
                    if (aac_codec) {
                        int len;
                        int data_size;
                        AVPacket packet;
    
                        avcodec_open(pCodecCtx, aac_codec);
    
                        av_init_packet(&packet);
                        av_read_frame(p_para->pFormatCtx, &packet);
    
                        data_size = sizeof(dec_buf);
                        len = avcodec_decode_audio3(pCodecCtx, (int16_t *)dec_buf, &data_size, &packet);
                        if (len < 0) {
                            log_print("[%s,%d] decode failed!\n", __func__, __LINE__);
                        }
    
                        avcodec_close(pCodecCtx);
                    }
                }
#endif
            }
            if ((p_para->astream_info.audio_format < 0) ||
                (p_para->astream_info.audio_format >= AFORMAT_MAX)) {
                p_para->astream_info.has_audio = 0;
                log_print("audio format not support!\n");
            } else if (p_para->astream_info.audio_format == AFORMAT_UNSUPPORT) {
                p_para->astream_info.has_audio = 0;
            }
        }

}
static void get_av_codec_type(play_para_t *p_para)
{
    AVFormatContext *pFormatCtx = p_para->pFormatCtx;
    AVStream *pStream;
    AVCodecContext  *pCodecCtx;
    int video_index = p_para->vstream_info.video_index;
    int audio_index = p_para->astream_info.audio_index;
    int sub_index = p_para->sstream_info.sub_index;
    log_print("[%s:%d]vidx=%d aidx=%d sidx=%d\n", __FUNCTION__, __LINE__, video_index, audio_index, sub_index);

    if (video_index != -1) {
        pStream = pFormatCtx->streams[video_index];
        pCodecCtx = pStream->codec;
        p_para->vstream_info.video_format   = video_type_convert(pCodecCtx->codec_id, 1);
        if (pFormatCtx->drmcontent) {
            log_print("[%s:%d]DRM content found, not support yet.\n", __FUNCTION__, __LINE__);
            p_para->vstream_info.video_format = VFORMAT_UNSUPPORT;
        }
        if (pCodecCtx->codec_id == CODEC_ID_FLV1) {
            pCodecCtx->codec_tag = CODEC_TAG_F263;
            p_para->vstream_info.flv_flag = 1;
        } else {
            p_para->vstream_info.flv_flag = 0;
        }
        if ((pCodecCtx->codec_id == CODEC_ID_MPEG1VIDEO)
            || (pCodecCtx->codec_id == CODEC_ID_MPEG2VIDEO)
            || (pCodecCtx->codec_id == CODEC_ID_MPEG2VIDEO_XVMC)) {
            if (p_para->stream_type == STREAM_PS) {
                mpeg_check_sequence(p_para);
            }
        }
        if (p_para->stream_type == STREAM_ES && pCodecCtx->codec_tag != 0) {
            p_para->vstream_info.video_codec_type = video_codec_type_convert(pCodecCtx->codec_tag);
        } else {
            p_para->vstream_info.video_codec_type = video_codec_type_convert(pCodecCtx->codec_id);
        }

        if ((p_para->vstream_info.video_format < 0) ||
            (p_para->vstream_info.video_format >= VFORMAT_MAX) ||
            (IS_NEED_VDEC_INFO(p_para->vstream_info.video_format) &&
             p_para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_UNKNOW)) {
            p_para->vstream_info.has_video = 0;
        } else if (p_para->vstream_info.video_format == VFORMAT_UNSUPPORT) {
            p_para->vstream_info.has_video = 0;
        }

        if (p_para->vstream_info.video_format == VFORMAT_VC1) {
            if (p_para->vstream_info.video_codec_type == VIDEO_DEC_FORMAT_WMV3) {
                if (pFormatCtx->video_avg_frame_time != 0) {
                    p_para->vstream_info.video_rate = pFormatCtx->video_avg_frame_time * 96 / 10000;
                }
                // WMV3 the last byte of the extradata is a version number,
                // 1 for the samples we can decode
                if (pCodecCtx->extradata && !(pCodecCtx->extradata[3] & 1)) { // this format is not supported
                    log_error("[%s]can only support wmv3 version number 1\n", __FUNCTION__);
                    p_para->vstream_info.has_video = 0;
                }
            }
        } else if (p_para->vstream_info.video_format == VFORMAT_SW) {
            p_para->vstream_info.has_video = 0;
        } else if (p_para->vstream_info.video_format == VFORMAT_MPEG4) {
            int warping_accuracy = (pCodecCtx->mpeg4_vol_sprite >> 16) & 0xff;
            int wrap_points = (pCodecCtx->mpeg4_vol_sprite >> 8) & 0xff;
            int vol_sprite = pCodecCtx->mpeg4_vol_sprite & 0xff;
            if (vol_sprite == 2) { // not support totally
                log_print("[%s:%d]mpeg4 vol sprite usage %d, GMC wrappoint %d, quater_sample %d, accuracy %d\n",
                          __FUNCTION__, __LINE__, vol_sprite, wrap_points, pCodecCtx->quater_sample, warping_accuracy);
                if ((wrap_points > 2) || ((wrap_points == 2) && pCodecCtx->quater_sample) || (warping_accuracy == 3)) {
                    p_para->vstream_info.has_video = 0;
                }
            }
            if (pCodecCtx->mpeg4_partitioned) {
                log_print("[%s:%d]mpeg4 partitioned frame, not supported\n", __FUNCTION__, __LINE__);
                p_para->vstream_info.has_video = 0;
            }
            if (VIDEO_DEC_FORMAT_MPEG4_3 == p_para->vstream_info.video_codec_type) {
                if (pCodecCtx->height > 720) {
                    log_print("[%s:%d]DIVX3 can not support upper 720p\n", __FUNCTION__, __LINE__);
                    p_para->vstream_info.has_video = 0;
                }
            }
           if ((VIDEO_DEC_FORMAT_MPEG4_3 == p_para->vstream_info.video_codec_type || VIDEO_DEC_FORMAT_MPEG4_4 == p_para->vstream_info.video_codec_type) && am_getconfig_bool("media.amplayer.nosupportdivx"))
            {
                log_print("[%s:%d] not support	DIVX \n", __FUNCTION__, __LINE__);
                p_para->vstream_info.has_video = 0;
	    }
        } else if (p_para->vstream_info.video_format == VFORMAT_H264) {
            if ((p_para->pFormatCtx) && (p_para->pFormatCtx->pb) && (p_para->pFormatCtx->pb->local_playback == 1)) {
                if (pCodecCtx && (pCodecCtx->profile != FF_PROFILE_H264_CONSTRAINED_BASELINE) && (pCodecCtx->profile > FF_PROFILE_H264_HIGH_10)) {
                    log_print("FF_PROFILE_H264_HIGH_10[0x%x] unsupport h264 profile [0x%x][%d] level[%d]\n", FF_PROFILE_H264_HIGH_10, pCodecCtx->profile, pCodecCtx->profile, pCodecCtx->level);
                    p_para->vstream_info.has_video = 0;
                }
            }
        }

        if (pCodecCtx && p_para->vstream_info.has_video) {
            p_para->vstream_info.video_pid      = (unsigned short)pStream->id;
            if (0 != pStream->time_base.den) {
                p_para->vstream_info.video_duration = ((float)pStream->time_base.num / pStream->time_base.den) * UNIT_FREQ;
                p_para->vstream_info.video_pts      = ((float)pStream->time_base.num / pStream->time_base.den) * PTS_FREQ;
            }
            p_para->vstream_info.video_width    = pCodecCtx->width;
            p_para->vstream_info.video_height   = pCodecCtx->height;
            p_para->vstream_info.video_ratio    = (pStream->sample_aspect_ratio.num << 16) | pStream->sample_aspect_ratio.den;
            p_para->vstream_info.video_ratio64  = (pStream->sample_aspect_ratio.num << 32) | pStream->sample_aspect_ratio.den;
            p_para->vstream_info.video_rotation_degree = pStream->rotation_degree;

            log_print("[%s:%d]vpid=0x%x,time_base=%d/%d,r_frame_rate=%d/%d ratio=%d/%d video_pts=%.3f\n", __FUNCTION__, __LINE__, \
                      p_para->vstream_info.video_pid, \
                      pCodecCtx->time_base.num, pCodecCtx->time_base.den, \
                      pStream->r_frame_rate.den, pStream->r_frame_rate.num, \
                      pStream->sample_aspect_ratio.num, pStream->sample_aspect_ratio.den, p_para->vstream_info.video_pts);

            if (0 != pCodecCtx->time_base.den) {
                p_para->vstream_info.video_codec_rate = (int64_t)UNIT_FREQ * pCodecCtx->time_base.num / pCodecCtx->time_base.den;
            }

            if (0 != pStream->r_frame_rate.num) {
                p_para->vstream_info.video_rate = (int64_t)UNIT_FREQ * pStream->r_frame_rate.den / pStream->r_frame_rate.num;
            }
            log_print("[%s:%d]video_codec_rate=%d,video_rate=%d\n", __FUNCTION__, __LINE__, p_para->vstream_info.video_codec_rate, p_para->vstream_info.video_rate);
            if ((p_para->pFormatCtx->pb != NULL && p_para->pFormatCtx->pb->is_slowmedia) && (p_para->vstream_info.video_format == VFORMAT_MPEG4) && (p_para->vstream_info.video_rate < 10)) {
                // in network playback. fast switch might causes stream video_rate info  not correct . then set it to 0.
                p_para->vstream_info.video_rate = 0;
                log_print(" video_rate might not be correct. set it to 0  video_rate =%d\n", p_para->vstream_info.video_rate);
            }
            if (p_para->vstream_info.video_format != VFORMAT_MPEG12) {
                p_para->vstream_info.extradata_size = pCodecCtx->extradata_size;
                p_para->vstream_info.extradata      = pCodecCtx->extradata;
            } else if (MKV_FILE == p_para->file_type) {
                p_para->vstream_info.extradata_size = pCodecCtx->extradata_size;
                p_para->vstream_info.extradata      = pCodecCtx->extradata;
            }

            if (pStream->start_time < 0) {  //fft:Only set this if you are absolutely 100% sure that the value you set it to really is the pts of the first frame.
                pStream->start_time = 0;
            }

            p_para->vstream_info.start_time = pStream->start_time * pStream->time_base.num * PTS_FREQ / pStream->time_base.den;

            /* added by Z.C for mov file frame duration */
            if ((p_para->file_type == MOV_FILE) || (p_para->file_type == MP4_FILE)) {
                if (pStream->nb_frames && pStream->duration && pStream->time_base.den && pStream->time_base.num) {
                    unsigned int fix_rate;
                    if ((0 != pStream->time_base.den) && (0 != pStream->nb_frames)) {
                        fix_rate = UNIT_FREQ * pStream->duration * pStream->time_base.num / pStream->time_base.den / pStream->nb_frames;
                    }
                    if ((fix_rate < UNIT_FREQ / 10) && (fix_rate > UNIT_FREQ / 60)) {
                        p_para->vstream_info.video_rate = fix_rate;
                        log_print("[%s:%d]video_codec_rate=%d,video_rate=%d\n", __FUNCTION__, __LINE__, p_para->vstream_info.video_codec_rate, p_para->vstream_info.video_rate);
                    }
                }
            } else if (p_para->file_type == FLV_FILE) {
                if (pStream->special_fps > 0 && pStream->special_fps < (float)100) {
                    p_para->vstream_info.video_rate = UNIT_FREQ / pStream->special_fps;
                }
                log_print("[%s:%d]pStream->special_fps=%f,video_rate=%d\n", __FUNCTION__, __LINE__, pStream->special_fps, p_para->vstream_info.video_rate);
            }
        }
    } else {
        p_para->vstream_info.has_video = 0;
        log_print("no video specified!\n");
    }
    if (audio_index != -1) {
        get_audio_type(p_para);
        pStream = pFormatCtx->streams[audio_index];
        pCodecCtx = pStream->codec;
        if (p_para->astream_info.has_audio) {
            if (0 != pStream->time_base.den) {
                p_para->astream_info.audio_duration = PTS_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
            }
            p_para->astream_info.start_time = pStream->start_time * pStream->time_base.num * PTS_FREQ / pStream->time_base.den;
        }
    } else {
        p_para->astream_info.has_audio = 0;
        log_print("no audio specified!\n");
    }
    if (sub_index != -1) {
        pStream = pFormatCtx->streams[sub_index];
        p_para->sstream_info.sub_pid = (unsigned short)pStream->id;
        p_para->sstream_info.sub_type = pStream->codec->codec_id;
        if (pStream->time_base.num && (0 != pStream->time_base.den)) {
            p_para->sstream_info.sub_duration = UNIT_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
            p_para->sstream_info.sub_pts = PTS_FREQ * ((float)pStream->time_base.num / pStream->time_base.den);
            p_para->sstream_info.start_time = pStream->start_time * pStream->time_base.num * PTS_FREQ / pStream->time_base.den;
            p_para->sstream_info.last_duration = 0;
        } else {
            p_para->sstream_info.start_time = pStream->start_time * PTS_FREQ;
        }
    } else {
        p_para->sstream_info.has_sub = 0;
    }
    return;
}

static void check_no_program(play_para_t *p_para)
{
    AVFormatContext *pFormat = p_para->pFormatCtx;
    AVStream *pStream;
    int get_audio_stream = 0, get_video_stream = 0, get_sub_stream = 0;;

    if (pFormat->nb_programs) {
        unsigned int i, j, k;

        /* set all streams to no_program */
        for (i = 0; i < pFormat->nb_streams; i++) {
            pFormat->streams[i]->no_program = 1;
        }

        /* check program stream */
        for (i = 0; i < pFormat->nb_programs; i++) {
            for (j = 0; j < pFormat->programs[i]->nb_stream_indexes; j++) {
                k = pFormat->programs[i]->stream_index[j];
                pFormat->streams[k]->no_program = 0;
                if ((!get_video_stream) && (pFormat->streams[k]->codec->codec_type == CODEC_TYPE_VIDEO)&&pFormat->streams[k]->stream_valid) {
                    get_video_stream = 1;
                }
                if ((!get_audio_stream) && (pFormat->streams[k]->codec->codec_type == CODEC_TYPE_AUDIO)&&pFormat->streams[k]->stream_valid) {
                    get_audio_stream = 1;
                }
                if ((!get_sub_stream) && (pFormat->streams[k]->codec->codec_type == CODEC_TYPE_SUBTITLE)&&pFormat->streams[k]->stream_valid) {
                    get_sub_stream = 1;
                }
            }
        }

        if (!get_video_stream) {
            for (i = 0; i < pFormat->nb_streams; i++) {
                if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
                    pFormat->streams[i]->no_program = 0;
                }
            }
        }

        if (!get_audio_stream) {
            for (i = 0; i < pFormat->nb_streams; i++) {
                if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
                    pFormat->streams[i]->no_program = 0;
                }
            }
        }

        if (!get_sub_stream) {
            for (i = 0; i < pFormat->nb_streams; i++) {
                if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_SUBTITLE) {
                    pFormat->streams[i]->no_program = 0;
                }
            }
        }
    }

    return;
}
static int check_same_program(play_para_t *p_para, int vproginx, int audiopid, int subtitlepid, int type)
{
    AVFormatContext *pFormat = p_para->pFormatCtx;
    AVStream *pStream;
    unsigned int i = 0, j = 0;

    for (i = 0; i < pFormat->programs[vproginx]->nb_stream_indexes; i++) {
        j = pFormat->programs[vproginx]->stream_index[i];
        pStream = pFormat->streams[j];
        if (type == CODEC_TYPE_AUDIO && (pStream->codec->codec_type == CODEC_TYPE_AUDIO) && pStream->id == audiopid) {
            return 1;
        }
        if (type == CODEC_TYPE_SUBTITLE && (pStream->codec->codec_type == CODEC_TYPE_SUBTITLE) && pStream->id == subtitlepid) {
            return 1;
        }
    }
    return 0;
}

static void get_ts_program(play_para_t *p_para, int program_num, ts_programe_info_t *ts_programe_info)
{
    int i, j, index;
    AVStream *pStream;
    AVProgram *pPrograms;
    AVFormatContext *pFormat; 
    ts_programe_detail_t *ts_programe_detail;
    AVDictionaryEntry *tag = NULL;

    pFormat = p_para->pFormatCtx;
    pPrograms = pFormat->programs[program_num];
    ts_programe_detail = &ts_programe_info->ts_programe_detail[ts_programe_info->programe_num];

    for (i = 0; i < pPrograms->nb_stream_indexes; i++) {
        index = pPrograms->stream_index[i];
        pStream = pFormat->streams[index];

        if (pStream->id == 0)//drop 0 pid
            continue;

        if (pStream->codec->codec_type == CODEC_TYPE_VIDEO) {
            ts_programe_detail->video_pid = pStream->id;
            ts_programe_detail->video_format = video_type_convert(pStream->codec->codec_id, 0);
            tag = av_dict_get(pPrograms->metadata, "service_name", NULL, 0);

            if ( tag != NULL && tag->value != NULL ) {
                char* strGB = tag->value;
                int lenSrc = strlen(tag->value);
                int lenSrc_o = lenSrc;
                int lenDst = lenSrc*5;
                int len =0;
                char* output_p  = (char*) malloc(lenDst);
                char* pFreeOut = output_p;

                iconv_t cd = iconv_open("UTF-8", "GBK");
                iconv(cd, &strGB, &lenSrc,  &output_p, &lenDst);
                iconv_close(cd);
                len = (lenSrc_o*5 - lenDst);
                if(len >=sizeof(ts_programe_detail->programe_name)){
                    len = sizeof(ts_programe_detail->programe_name)-1;
                }
                memcpy(&(ts_programe_detail->programe_name), pFreeOut, len);
                free(pFreeOut);
            }
        } else if (pStream->codec->codec_type == CODEC_TYPE_AUDIO) {
            for (j = 0; j < MAX_AUDIO_STREAMS; j++) {
                if (ts_programe_detail->audio_pid[j] == pStream->id)
                    break;
                ts_programe_detail->audio_format[ts_programe_detail->audio_track_num] = audio_type_convert(pStream->codec->codec_id, p_para->file_type, 0);
                ts_programe_detail->audio_pid[ts_programe_detail->audio_track_num] = pStream->id;
                ts_programe_detail->audio_track_num++;
                j++;
                break;
            }
        }
    }
    //ts_programe_info->programe_num++;
}

static int ts_program_exist(ts_programe_info_t *ts_programe_info, int pid)
{
    int i;
    for (i = 0; i < ts_programe_info->programe_num; i++) {
        if (ts_programe_info->ts_programe_detail[i].video_pid == pid)
            return 0;
    }

    return -1;
}

int player_get_ts_index_of_pid(play_para_t *p_para, int pid){
    int i;
    AVStream *pStream;
    AVFormatContext *pFormat = p_para->pFormatCtx;

    for (i = 0; i < pFormat->nb_streams; i++) {
        pStream = pFormat->streams[i];
        if (pid == pStream->id)
            return i;
    }

    return 0xffff;
}

int player_get_ts_pid_of_index(play_para_t *p_para, int index){
    int i;
    AVStream *pStream;
    AVFormatContext *pFormat = p_para->pFormatCtx;

    if (index < pFormat->nb_streams && index >= 0) {
        pStream = pFormat->streams[index];
        return (pStream != NULL)?(pStream->id):(-1);
    }

    return -1;
}
static void get_ts_program_info(play_para_t *p_para, ts_programe_info_t *ts_programe_info)
{
    int i, j, index;
    AVStream *pStream;
    int ts_program;
    int info_exist = !ts_programe_info->programe_num;
    AVFormatContext *pFormat = p_para->pFormatCtx;

    //ts_programe_info = &p_para->media_info.ts_programe_info;
   // memset(ts_programe_info, 0, sizeof(ts_programe_info_t));

    for (i = 0; i < pFormat->nb_programs; i++) {
        for (j = 0; j < pFormat->programs[i]->nb_stream_indexes; j++) {
            index = pFormat->programs[i]->stream_index[j];
            pStream = pFormat->streams[index];
            if (pStream->codec->codec_type == CODEC_TYPE_VIDEO) {
                ts_program = ts_program_exist(ts_programe_info, pStream->id);
                if (ts_program != 0 && pStream->codec->width != 0 && pStream->codec->height != 0){
                    get_ts_program(p_para, i, ts_programe_info);
                    if(info_exist)
                        ts_programe_info->programe_num++;
                }
                break;
            }
        }
    }
}
int player_check_program_change(play_para_t *p_para)
{
    int afmt_change = 0;
    int vfmt_change = 0;
    int vpid_change = 0;
    int apid_change = 0;
    int i = 0, j = 0;
    ts_programe_info_t new_programe_info;
    ts_programe_detail_t *ts_programe_detail = NULL;
    ts_programe_info_t* old_programe_info = &p_para->media_info.ts_programe_info;
    
    if (!am_getconfig_bool("libplayer.progchange.enable")) {
        return 0;
    }

    if(!p_para->pFormatCtx) {
        return 0;
    }
    if(!av_is_segment_media(p_para->pFormatCtx)){
        //log_print("%s,only support by hls\n",__func__);
        return 0;
    }
    mstream_info_t* new_stream_info = &p_para->media_info.ts_programe_info.new_stream_info;
    memset(&new_programe_info, 0, sizeof(ts_programe_info_t));
    if(p_para->playctrl_info.switch_param_flag){
        if(p_para->start_param->is_livemode != 1){
            if( !p_para->playctrl_info.read_end_flag && (get_player_state(p_para) == PLAYER_BUFFERING)){
                p_para->playctrl_info.switch_ts_program_flag=1;
                p_para->playctrl_info.reset_flag = 1;
                p_para->playctrl_info.end_flag = 1;
                p_para->playctrl_info.time_point = ((p_para->state.current_time+10)/10)*10;//for hls
                log_print("%s, switch prog reseting point:%f\n",__func__,p_para->playctrl_info.time_point);
            }else if (p_para->playctrl_info.read_end_flag && (get_player_state(p_para) != PLAYER_PAUSE)&&(p_para->playctrl_info.video_low_buffer ||
                    p_para->playctrl_info.audio_low_buffer)) {
                p_para->playctrl_info.switch_ts_program_flag=1;
                p_para->playctrl_info.reset_flag = 1;
                p_para->playctrl_info.end_flag = 1;
                p_para->playctrl_info.time_point =  ((p_para->state.current_time+10)/10)*10;//for hls
                log_print("%s,1 switch prog reseting point:%f\n",__func__,p_para->playctrl_info.time_point);
            } else {
                //don't reset just waiting
                return 0;
            }
        } else {
            p_para->playctrl_info.switch_ts_program_flag=1;
            p_para->playctrl_info.reset_flag = 1;
            p_para->playctrl_info.end_flag = 1;
            p_para->playctrl_info.time_point = -1;//for live
            log_print("%s,live switch prog reseting point:%f\n",__func__,p_para->playctrl_info.time_point);
        }
    }

    get_ts_program_info(p_para, &new_programe_info);
    
    if(new_programe_info.programe_num != 1){
        log_print("%s, programe num:%d is not 1, don't support\n",__func__,new_programe_info.programe_num);
        return 0;
    }
    ts_programe_detail = &new_programe_info.ts_programe_detail[0];
    if(((ts_programe_detail->video_format)<VFORMAT_UNKNOWN)
        ||((ts_programe_detail->video_format)>VFORMAT_UNSUPPORT)) {
        log_print("%s, prog_detail:%p,video format %d error,num:%d\n",__func__,ts_programe_detail, ts_programe_detail->video_format,new_programe_info.programe_num);
        return 0;
    }
    if(new_programe_info.programe_num == old_programe_info->programe_num) {
        //Fix me
        if(ts_programe_detail->audio_track_num!=old_programe_info->ts_programe_detail[0].audio_track_num){
            log_print("%s, audio track num:[%d->%d] diff is not support\n",__func__,old_programe_info->ts_programe_detail[0].audio_track_num,
                ts_programe_detail->audio_track_num);
            return 0;
        }
        if (ts_programe_detail->video_pid!=old_programe_info->ts_programe_detail[0].video_pid){
            vpid_change =1;
            log_print("%s, vpid_change,(%d->%d)\n",__func__,old_programe_info->ts_programe_detail[0].video_pid,ts_programe_detail->video_pid);
        }
        if (ts_programe_detail->video_format!=old_programe_info->ts_programe_detail[0].video_format) {
            vfmt_change =1;
            log_print("%s, vfmt_change,(%d->%d)\n",__func__,old_programe_info->ts_programe_detail[0].video_format,ts_programe_detail->video_format);
        }
        for(i;i<ts_programe_detail->audio_track_num;i++){
            if(ts_programe_detail->audio_pid[i]!=old_programe_info->ts_programe_detail[0].audio_pid[i]){
                apid_change = 1;
                log_print("%s, apid_change,(%d->%d)\n",__func__,old_programe_info->ts_programe_detail[0].audio_pid[i],ts_programe_detail->audio_pid[i]);
            }
            if(ts_programe_detail->audio_format[i]!=old_programe_info->ts_programe_detail[0].audio_format[i]){
                afmt_change = 1;
                log_print("%s, afmt_change,(%d->%d)\n",__func__,old_programe_info->ts_programe_detail[0].audio_format[i],ts_programe_detail->audio_format[i]);
            }
        }
    }
    if((afmt_change||apid_change||vfmt_change||vpid_change)){
        p_para->playctrl_info.switch_param_flag =1; 
        p_para->media_info.ts_programe_info.afmt_change = afmt_change;
        p_para->media_info.ts_programe_info.vfmt_change = vfmt_change;
        p_para->media_info.ts_programe_info.programe_num = new_programe_info.programe_num;
        memcpy(p_para->media_info.ts_programe_info.ts_programe_detail,new_programe_info.ts_programe_detail,MAX_VIDEO_STREAMS*sizeof(ts_programe_detail_t));
        new_stream_info->cur_video_index = player_get_ts_index_of_pid(p_para,ts_programe_detail->video_pid);
        new_stream_info->cur_audio_index = player_get_ts_index_of_pid(p_para,ts_programe_detail->audio_pid[0]);
    }
    return 0;
}

static int sub_support(int codec_id)
{
    if(codec_id == CODEC_ID_DVB_TELETEXT)
        return 0;
    return 1;
}

static int dts_audio_codec_support()
{
    char buf[8];
    int dts_en = 0;
    amsysfs_get_sysfs_str(DTS_ENABLE_PATH, buf, sizeof(buf));
    if (sscanf(buf, "0x%x", &dts_en) < 1) {
        log_print("unable to get dts enable port: %s", buf);
        return 0;
    }
    log_print("%s, dts_en=%d,buf: %s\n",__func__,dts_en, buf);
    return dts_en;
}

static int dobly_audio_codec_support()
{
    char buf[8];
    int dobly_en = 0;
    amsysfs_get_sysfs_str(DOBLY_ENABLE_PATH, buf, sizeof(buf));
    if (sscanf(buf, "0x%x", &dobly_en) < 1) {
        log_print("unable to get dobly enable port: %s", buf);
        return 0;
    }
    log_print("%s, dobly_en=%d,buf: %s\n",__func__,dobly_en, buf);
    return dobly_en;
}

static int get_digitalraw_mode(void)
{
    return amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");
}
static int prefer_ac3(aformat_t audio_format, int i)
{
    int temp_aidx = -1;
    int ac3_first = 0;
    ac3_first = am_getconfig_bool("media.amplayer.default_ac3");
    if(!ac3_first)
        return temp_aidx;
    if (audio_format == AFORMAT_AC3) {
          if (dobly_audio_codec_support() || get_digitalraw_mode() > 0) {
              temp_aidx = i;
              log_print("[%s],Audio Format is AC3, select it\n",__func__);
          } else {
              log_print("[%s],Audio Format is AC3, but chip not support or pass through\n",__func__);
          }

    } else if (audio_format == AFORMAT_EAC3) {
      if (dobly_audio_codec_support() || get_digitalraw_mode() == 2) {
          temp_aidx = i;
          log_print("[%s],Audio Format is EAC3, , select it\\n",__func__);
      } else {
          log_print("[%s],Audio Format is EAC3, but chip not support or pass through\n",__func__);
      }

    }
    return temp_aidx;
}

static void get_stream_info(play_para_t *p_para)
{
    unsigned int i, k, j;
    AVFormatContext *pFormat = p_para->pFormatCtx;
    AVStream *pStream;
    AVCodecContext *pCodec;
    AVDictionaryEntry *t;
    int video_index = p_para->vstream_info.video_index;
    int audio_index = p_para->astream_info.audio_index;
    int sub_index = p_para->sstream_info.sub_index;
    int temp_vidx = -1, temp_aidx = -1, temp_sidx = -1;
    int first_aidx = -1;
    int temppid = -1;
    int bitrate = 0;
    int read_packets = 0;
    int ret = 0;
    aformat_t audio_format;
    int vcodec_noparameter_idx = -1;
    int acodec_noparameter_idx = -1;
    int acodec_noparameter_idx_nb_frames = 0;
    int astream_id[ASTREAM_MAX_NUM] = {0};
    int new_flag = 1;
    int unsupported_video = 0;
    int tsvideopid = -1;
    int tsaudiopid = -1;
    int tssubtitlepid = -1;
    int tsvproginx = -1;

	const char * cpu_type = "/sys/class/cputype/cputype";
	char c_cpu_type[5] = {0};
	char *valstr;

	valstr = c_cpu_type;
    p_para->first_index = pFormat->first_index;

    /* caculate the stream numbers */
    p_para->vstream_num = 0;
    p_para->astream_num = 0;
    p_para->sstream_num = 0;

    check_no_program(p_para);
    ts_programe_info_t* ts_programe_info = &p_para->media_info.ts_programe_info;
    memset(ts_programe_info, 0, sizeof(ts_programe_info_t));
    get_ts_program_info(p_para, ts_programe_info);

    //for mutil programs TS, get the first video stream PID and program index, we will filter all the audio & subtitle streams are not in the same program.
    if (!strcmp(pFormat->iformat->name, "mpegts") && pFormat->nb_programs > 1) {
        for (i = 0; i < pFormat->nb_streams; i++) {
            if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
                tsvideopid = pFormat->streams[i]->id;
                break;
            }
        }
        if (tsvideopid != -1) {
            for (i = 0; i < pFormat->nb_programs; i++) {
                for (j = 0; j < pFormat->programs[i]->nb_stream_indexes; j++) {
                    k = pFormat->programs[i]->stream_index[j];
                    pStream = pFormat->streams[k];
                    if ((pStream->codec->codec_type == CODEC_TYPE_VIDEO) && pStream->id == tsvideopid) {
                        tsvproginx = i;
                        break;
                    }
                }
                if (tsvproginx != -1) {
                    break;
                }
            }
        }
    }

    for (i = 0; i < pFormat->nb_streams; i++) {
        pStream = pFormat->streams[i];

        if (pStream->no_program || !pStream->stream_valid) {
            log_print("[%s:%d]stream %d no_program:%d, stream_valid:%d, \n", __FUNCTION__, __LINE__, i, pStream->no_program, pStream->stream_valid);
            continue;
        }

        pCodec = pStream->codec;
        if (pCodec->codec_type == CODEC_TYPE_VIDEO) {
            p_para->vstream_num ++;
            if (p_para->file_type == RM_FILE) {
                /* find max bitrate */
                if (pCodec->bit_rate > bitrate) {
                    /* support  RV10  RV20 RV30 and RV40 */
                    if ((pCodec->codec_id == CODEC_ID_RV30)
                        || (pCodec->codec_id == CODEC_ID_RV40)
                       	|| (pCodec->codec_id == CODEC_ID_RV20)
                       	|| (pCodec->codec_id == CODEC_ID_RV10)) {
                        ret = try_decode_picture(p_para, i);
                        if (ret == 0) {
                            bitrate = pCodec->bit_rate;
                            temp_vidx = i;
                        } else if (ret > read_packets) {
                            read_packets = ret;
                            temp_vidx = i;
                        }
                    }
                }
            } else {
                if (temp_vidx == -1) {
                    //why disable video with encrypt,it will affect DTS certification ,so disable it ????,someone can do further check
                    if (0) { // (strcmp(pFormat->iformat->name, "mpegts") == 0 && pStream->encrypt) {
                        //mpegts encrypt
                        log_print("pid=%d crytion\n", pStream->id);
                    } else {
                        if (!strcmp(pFormat->iformat->name, "mpegts") && !check_codec_parameters_ex(pStream->codec, 1)) {
                            //Maybe all vcodec without codec parameter but at least choose one stream,save it.Need double check at last.
                            if (vcodec_noparameter_idx == -1) {
                                vcodec_noparameter_idx = i;
                            }
                            log_print("video pid=%d has not codec parameter\n", pStream->id);
                        } else {
                            temp_vidx = i;
                        }
                    }
                }
            }
        } else if (pCodec->codec_type == CODEC_TYPE_AUDIO) {
            /*
            * firstly get the disabled audio format, if the current format is disabled, parse the next directly
            */
            int filter_afmt = PlayerGetAFilterFormat("media.amplayer.disable-acodecs");
            audio_format = audio_type_convert(pCodec->codec_id, p_para->file_type, 1);
            if (((1 << audio_format) & filter_afmt) != 0) {
                pStream->stream_valid = 0;
                log_print("## filtered format audio_format=%d,i=%d,----\n", audio_format, i);
                continue;
            }
#if 0
            // check if current audio stream exist already.
            for (k = 0; k < p_para->astream_num; k++) {
                if (pStream->id == astream_id[k]) {
                    new_flag = 0;
                    break;
                }
            }
#endif
            if (!new_flag) {
                log_print("## [%s:%d] stream i=%d is the same to k=%d, id=%d,\n", __FUNCTION__, __LINE__, i, k, pStream->id);
                new_flag = 1;
                pStream->stream_valid = 0;
                continue;
            }
            if (tsvproginx != -1 && !strcmp(pFormat->iformat->name, "mpegts") && pFormat->nb_programs > 1) {
                tsaudiopid = pStream->id;
                if (!check_same_program(p_para, tsvproginx, tsaudiopid, tssubtitlepid, CODEC_TYPE_AUDIO)) {
                    pStream->stream_valid = 0;
                    log_print("## filtered format audio_format that not in the same program=%d,i=%d,----\n", audio_format, i);
                    continue;
                }
            }
            astream_id[p_para->astream_num] = pStream->id;
            p_para->astream_num ++;
#if 0
            //not support blueray stream,one PID has two audio track(truehd+ac3)
            if (strcmp(pFormat->iformat->name, "mpegts") == 0) {
                if (pCodec->codec_id == CODEC_ID_TRUEHD) {
                    temppid = pStream->id;
                    log_print("temppidstream: %s:%d\n", pFormat->iformat->name, temppid);
                } else if (pCodec->codec_id == CODEC_ID_AC3 && pStream->id == temppid) {
                    audio_format = AFORMAT_UNSUPPORT;
                    log_print("unsupport truehd and AC-3 with the same pid\n");
                }
            }
#endif
            /*audio first select ac3 index*/
            int choose_ac3 = -1;
            choose_ac3 = prefer_ac3(audio_format, i);
            if(choose_ac3 != -1)
                temp_aidx  = choose_ac3;

            if (temp_aidx == -1 && audio_format != AFORMAT_UNSUPPORT) {
                if (strcmp(pFormat->iformat->name, "mpegts") == 0
                    && pStream->encrypt
                    && !am_getconfig_bool("libplayer.ts.udrm.enable")) {
                    //mpegts encrypt
                    log_print("pid=%d crytion\n", pStream->id);
                } else {
                    if (!strcmp(pFormat->iformat->name, "mpegts") && !check_codec_parameters_ex(pStream->codec, 0)) {
                        //Maybe all acodec without codec parameter but at least choose one stream,save it.Need double check at last.
                        if (acodec_noparameter_idx == -1) {
                            acodec_noparameter_idx = i;
                            acodec_noparameter_idx_nb_frames = pStream->codec_info_nb_frames;
                        }
                        if (pStream->codec_info_nb_frames > acodec_noparameter_idx_nb_frames) {
                            acodec_noparameter_idx = i;
                            acodec_noparameter_idx_nb_frames = pStream->codec_info_nb_frames;
                        }
                        log_print("audio pid=%d has not codec parameter,i=%d\n", pStream->id,i);
                    } else {
                        if (audio_format == AFORMAT_DTS) {
                            if (dts_audio_codec_support() || get_digitalraw_mode() > 0) {
                                temp_aidx = i;
                            } else {
                                log_print("[%s],Audio Format is DTS, but chip not support\n",__func__);
                            }
                        } else if (audio_format == AFORMAT_AC3) {
                            if (dobly_audio_codec_support() || get_digitalraw_mode() > 0) {
                                temp_aidx = i;
                            } else {
                                log_print("[%s],Audio Format is AC3, but chip not support or pass through\n",__func__);
                            }

                        } else if (audio_format == AFORMAT_EAC3) {
                            if (dobly_audio_codec_support() || get_digitalraw_mode() == 2) {
                                temp_aidx = i;
                            } else {
                                log_print("[%s],Audio Format is EAC3, but chip not support or pass through\n",__func__);
                            }

                        } else if (audio_format == AFORMAT_TRUEHD) {
                            if (get_digitalraw_mode() == 2) {
                                temp_aidx = i;
                            } else {
                                log_print("[%s],Audio Format is truehd, not pass through\n",__func__);
                            }
                        } else {
                            temp_aidx = i;
                        }

                    }

                    if (first_aidx == -1) {
                        first_aidx = i;
                    }

                }
            }
            char prop_value[PROPERTY_VALUE_MAX];
            int value = -1;
            if(property_get("audio.track.default.language", prop_value, NULL) > 0){
                value = atoi(prop_value);
            }
            
            /*
            audio.track.default.language  0   chinese first
            audio.track.default.language  1   english first
            */
            char *language[2]={"chi", "eng"}; 
            log_print("[%s:%d]audio.track.default.language is %s\n", __FUNCTION__,  __LINE__, prop_value);
            /* find chinese language audio track */
            if ( t = av_dict_get(pStream->metadata, "language", NULL, 0)) {
                  log_print("[%s:%d]language is %s\n", __FUNCTION__,  __LINE__, prop_value);
                if(value >=0 &&value <=1){
                    if (audio_format != AFORMAT_UNSUPPORT && !strncmp(t->value, language[value], 3)) {
                        if (temp_aidx >= 0 && (t = av_dict_get(pFormat->streams[temp_aidx]->metadata, "language", NULL, 0))) {
                            if (!strncmp(t->value, language[value], 3)) {
                                log_print("[%s:%d]already find %s language track, not change :key=%s value=%s audio track, %d\n", __FUNCTION__,  __LINE__, language[value],t->key, t->value, temp_aidx);
                            } else {
                                temp_aidx = i;
                                log_print("[%s:%d]find %s language track,change to:key=%s value=%s audio track, %d temp_aidx%d\n", __FUNCTION__,  __LINE__,language[value], t->key, t->value, i, temp_aidx);
                            }
                        }
                    }
            }
         }
        } else if (pCodec->codec_type == CODEC_TYPE_SUBTITLE) {
            if (tsvproginx != -1 && !strcmp(pFormat->iformat->name, "mpegts") && pFormat->nb_programs > 1) {
                tssubtitlepid = pStream->id;
                if (!check_same_program(p_para, tsvproginx, tsaudiopid, tssubtitlepid, CODEC_TYPE_SUBTITLE)) {
                    pStream->stream_valid = 0;
                    log_print("## filtered format subtitle that not in the same program=%d,i=%d,----\n", tssubtitlepid, i);
                    continue;
                }
            }
            // sub support check
            if(sub_support(pCodec->codec_id) == 0)
                continue;
            p_para->sstream_num ++;
            if (temp_sidx == -1) {
                temp_sidx = i;
            }
        }
    }
    //double check for mpegts
    if (!strcmp(pFormat->iformat->name, "mpegts") && p_para->vstream_num >= 1 && temp_vidx == -1 && vcodec_noparameter_idx != -1) {
        temp_vidx = vcodec_noparameter_idx;
    }
    if (!strcmp(pFormat->iformat->name, "mpegts") && p_para->astream_num >= 1 && temp_aidx == -1 && acodec_noparameter_idx != -1) {
        temp_aidx = acodec_noparameter_idx;
    }

    if (temp_aidx == -1 && p_para->astream_num >= 1) {
        temp_aidx = first_aidx;
    }

    if (p_para->vstream_num >= 1) {
        p_para->vstream_info.has_video = 1;
    } else {
        p_para->vstream_info.has_video = 0;
        p_para->vstream_info.video_format = -1;
    }

    if (p_para->astream_num >= 1) {
        p_para->astream_info.has_audio = 1;
    } else {
        p_para->astream_info.has_audio = 0;
        p_para->astream_info.audio_format = -1;
    }

    p_para->sstream_num = 0;
    if (p_para->sstream_num >= 1) {
        p_para->sstream_info.has_sub = 1;
    } else {
        p_para->sstream_info.has_sub = 0;
    }
    
    if ((p_para->vstream_num >= 1) ||
        (p_para->astream_num >= 1) ||
        (p_para->sstream_num >= 1)) {
        if ((video_index > (p_para->vstream_num + p_para->astream_num)) || (video_index < 0)) {
            video_index = temp_vidx;
        }

        if (audio_index > (p_para->vstream_num + p_para->astream_num) || audio_index < 0) {
            audio_index = temp_aidx;
        }

        if ((sub_index > p_para->sstream_num) || (sub_index < 0)) {
            sub_index = temp_sidx;
        }
    }
    if (p_para->astream_info.has_audio && audio_index != -1) {
        p_para->astream_info.audio_channel = pFormat->streams[audio_index]->codec->channels;
        p_para->astream_info.audio_samplerate = pFormat->streams[audio_index]->codec->sample_rate;
    }

    p_para->vstream_info.video_index = video_index;
    int test_aidx = am_getconfig_int_def("media.amplayer.aidx", 0);
    if (p_para->astream_num > 1 && test_aidx > 0)
        audio_index = test_aidx;

    p_para->astream_info.audio_index = audio_index;
    p_para->sstream_info.sub_index = sub_index;
    log_print("Video index %d and Audio index %d sub index %d to be played (index -1 means no stream)\n", video_index, audio_index, sub_index);
    if (p_para->sstream_info.has_sub) {
        log_print("Subtitle index %d detected\n", sub_index);
    }

    get_av_codec_type(p_para);

    if (p_para->stream_type == STREAM_RM && video_index != -1) {
        if (p_para->pFormatCtx->streams[video_index]->stream_offset > 0) {
            p_para->data_offset = p_para->pFormatCtx->streams[video_index]->stream_offset;
        } else {
            p_para->data_offset = p_para->pFormatCtx->data_offset;
        }
        log_print("[%s:%d]real offset %lld\n", __FUNCTION__, __LINE__, p_para->data_offset);

        if (p_para->vstream_info.video_height * p_para->vstream_info.video_width > 1936 * 1088) {
            log_print("[%s:%d]real video_height=%d, exceed 1080p not support!\n", __FUNCTION__, __LINE__, p_para->vstream_info.video_height);
            p_para->vstream_info.has_video = 0;
        }
    } else {
        p_para->data_offset = p_para->pFormatCtx->data_offset;
        log_print("[%s:%d]data start offset %lld\n", __FUNCTION__, __LINE__, p_para->data_offset);
    }


    if (video_index != -1) {
        if (p_para->vstream_info.video_format == VFORMAT_H264) {
            if ((p_para->vstream_info.video_width > 4096) ||
                (p_para->vstream_info.video_height > 2304)) {
                unsupported_video = 1;
                log_print("[%s:%d] H.264 video profile not supported", __FUNCTION__, __LINE__);
            } else if ((p_para->vstream_info.video_width * p_para->vstream_info.video_height) > (1920 * 1088)) {
                if (p_para->vdec_profile.h264_para.support_4k) {
                    p_para->vstream_info.video_format = VFORMAT_H264;
                } else if (p_para->vdec_profile.h264_4k2k_para.exist) {
                    p_para->vstream_info.video_format = VFORMAT_H264_4K2K;
                    log_print("H.264 4K2K video format applied.");
                } else {
                    if (p_para->vdec_profile.h264_4k2k_para.exist) {
                        p_para->vstream_info.video_format = VFORMAT_H264_4K2K;
                        log_print("H.264 4K2K video format applied.");
                    } else {
                        unsupported_video = 1;
                        log_print("[%d x %d] H.264 video profile not supported", p_para->vstream_info.video_width, p_para->vstream_info.video_height);
                    }
                }
            }
        } else if (p_para->vstream_info.video_format == VFORMAT_MPEG4) {
            if ((p_para->vstream_info.video_width * p_para->vstream_info.video_height) > (2048 * 1152)) {
                unsupported_video = 1;
                /*mpeg4 can support 2048x1152.
                more bigger?
                */
            }
        } else if (p_para->vstream_info.video_format == VFORMAT_AVS) {
            if (p_para->pFormatCtx->streams[video_index]->codec->profile == 1
                && !p_para->vdec_profile.avs_para.support_avsplus) {
                unsupported_video = 1;
                log_print("[%s:%d]avs+, not support now!\n", __FUNCTION__, __LINE__);
            }
        } else {
            if (p_para->vstream_info.video_format == VFORMAT_HEVC) {
                unsupported_video = p_para->pFormatCtx->streams[video_index]->codec->long_term_ref_pic == 1;
                if (unsupported_video) {
                    log_print("[%s:%d]hevc long term ref pic, not support now!\n", __FUNCTION__, __LINE__);
                }
                if (!unsupported_video) {
                    unsupported_video = (p_para->pFormatCtx->streams[video_index]->codec->bit_depth == 9 &&
                                         !p_para->vdec_profile.hevc_para.support_9bit) ||
                                        (p_para->pFormatCtx->streams[video_index]->codec->bit_depth == 10 &&
                                         !p_para->vdec_profile.hevc_para.support_10bit);
                    if (unsupported_video) {
                        log_print("[%s]hevc 9/10 bit profile, not support for this chip configure!,bit_depth=%d\n", __FUNCTION__,
                                  p_para->pFormatCtx->streams[video_index]->codec->bit_depth);
                    }
                }
            }
            if ((p_para->vstream_info.video_width > 1920) ||
                (p_para->vstream_info.video_height > 1088)) {
                if (p_para->vstream_info.video_format == VFORMAT_HEVC ) {
                    unsupported_video = (!p_para->vdec_profile.hevc_para.support4k | unsupported_video);
                } else {
			get_sysfs_str(cpu_type, valstr, 5);
			if((valstr[0] == '9') && (p_para->vstream_info.video_format == VFORMAT_REAL)) 
			/* 905/905L/905M chips support 1080p RMVB hard decoding */
			    unsupported_video = 0;
			else
			    unsupported_video = 1;
                }
            } else if (p_para->vstream_info.video_format == VFORMAT_VC1) {
                if ((!p_para->vdec_profile.vc1_para.interlace_enable) &&
                    (p_para->pFormatCtx->streams[video_index]->codec->frame_interlace)) {
                    unsupported_video = 1;
                    log_print("[%s:%d]vc1 interlace video, not support!\n", __FUNCTION__, __LINE__);
                }
                if (p_para->pFormatCtx->streams[video_index]->codec->vc1_profile == 2) {
                    // complex profile, we don't support now
                    unsupported_video = 1;
                    log_print("[%s:%d]vc1 complex profile video, not support!\n", __FUNCTION__, __LINE__);
                }
            }

        }

        if (unsupported_video) {
            log_error("[%s]can't support exceeding video profile\n", __FUNCTION__);
            set_player_error_no(p_para, PLAYER_UNSUPPORT_VIDEO);
            update_player_states(p_para, 1);
            p_para->vstream_info.has_video = 0;
            p_para->vstream_info.video_index = -1;
        }
    }

    return;
}

static int set_decode_para(play_para_t*am_p)
{
    signed short audio_index = am_p->astream_info.audio_index;
    int ret = -1;
    int rev_byte = 0;
    int total_rev_bytes = 0;
    vformat_t vfmt;
    aformat_t afmt;
    int filter_vfmt = 0, filter_afmt = 0;
    unsigned char* buf;
    ByteIOContext *pb = am_p->pFormatCtx->pb;
    int prop = -1;
    char dts_value[PROPERTY_VALUE_MAX];
    char ac3_value[PROPERTY_VALUE_MAX];
    unsigned int video_codec_id;

    get_stream_info(am_p);
    log_print("[%s:%d]has_video=%d vformat=%d has_audio=%d aformat=%d", __FUNCTION__, __LINE__, \
              am_p->vstream_info.has_video, am_p->vstream_info.video_format, \
              am_p->astream_info.has_audio, am_p->astream_info.audio_format);

    filter_vfmt = PlayerGetVFilterFormat(am_p);

    if (((1 << am_p->vstream_info.video_format) & filter_vfmt) != 0) {
        log_error("Can't support video codec! filter_vfmt=%x vfmt=%x  (1<<vfmt)=%x\n", \
                  filter_vfmt, am_p->vstream_info.video_format, (1 << am_p->vstream_info.video_format));
        if (VFORMAT_H264MVC == am_p->vstream_info.video_format) {
            am_p->vstream_info.video_format = VFORMAT_H264; /*if kernel not support mvc,just playing as 264 now.*/
            if ((am_p->vstream_info.video_width > 1920) ||
                (am_p->vstream_info.video_height > 1088)) {
                if (am_p->vdec_profile.h264_para.support_4k) {
                    am_p->vstream_info.video_format = VFORMAT_H264;
                } else if (am_p->vdec_profile.h264_4k2k_para.exist) {
                    am_p->vstream_info.video_format = VFORMAT_H264_4K2K;
                    log_print("H.264 4K2K video format applied.");
                } else {
                    if (am_p->vdec_profile.h264_4k2k_para.exist) {
                        am_p->vstream_info.video_format = VFORMAT_H264_4K2K;
                        log_print("H.264 4K2K video format applied.");
                    } else {
                        am_p->vstream_info.has_video = 0;
                        set_player_error_no(am_p, PLAYER_UNSUPPORT_VCODEC);
                        update_player_states(am_p, 1);
                        log_print("[%s:%d] H.264 video profile not supported");
                    }
                }
            }
        } else {
            am_p->vstream_info.has_video = 0;
            set_player_error_no(am_p, PLAYER_UNSUPPORT_VCODEC);
            update_player_states(am_p, 1);
        }
    }
#if 0
    /*
    * in the get_stream_info function,an enabled audio format would be selected according to the
    * media.amplayer.disable-vcodecs property
    */
    filter_afmt = PlayerGetAFilterFormat();
    if (((1 << am_p->astream_info.audio_format) & filter_afmt) != 0) {
        log_error("Can't support audio codec! filter_afmt=%x afmt=%x  (1<<afmt)=%x\n", \
                  filter_afmt, am_p->astream_info.audio_format, (1 << am_p->astream_info.audio_format));
        am_p->astream_info.has_audio = 0;
        set_player_error_no(am_p, PLAYER_UNSUPPORT_ACODEC);
        update_player_states(am_p, 1);
    }
#endif
    if (am_p->pFormatCtx->drmcontent) {
        set_player_error_no(am_p, DRM_UNSUPPORT);
        update_player_states(am_p, 1);
        log_error("[%s:%d]Can't support drm yet!\n", __FUNCTION__, __LINE__);
        return PLAYER_UNSUPPORT;
    }

    if (am_p->playctrl_info.no_video_flag) {
        am_p->vstream_info.has_video = 0;
        set_player_error_no(am_p, PLAYER_SET_NOVIDEO);
        update_player_states(am_p, 1);
    } else if (!am_p->vstream_info.has_video) {
        if (am_p->file_type == RM_FILE) {
            log_print("[%s %d]SUPPORT RM FILE WITHOUT VIDEO USING FFMPEG SOFTDEMUX AND DECODER...\n", __FUNCTION__, __LINE__);
            //return PLAYER_UNSUPPORT;
        } else if (am_p->astream_info.has_audio) {
            if (IS_VFMT_VALID(am_p->vstream_info.video_format)) {
                set_player_error_no(am_p, PLAYER_UNSUPPORT_VIDEO);
                update_player_states(am_p, 1);
            } else {
                set_player_error_no(am_p, PLAYER_NO_VIDEO);
                update_player_states(am_p, 1);
            }
        } else {
            if (IS_AFMT_VALID(am_p->astream_info.audio_format)) {
                set_player_error_no(am_p, PLAYER_UNSUPPORT_AUDIO);
                update_player_states(am_p, 1);
            } else {
                set_player_error_no(am_p, PLAYER_NO_AUDIO);
                update_player_states(am_p, 1);
            }
            log_error("[%s:%d]Can't support the file!\n", __FUNCTION__, __LINE__);
            return PLAYER_UNSUPPORT;
        }
    }


    filter_afmt = PlayerGetAFilterFormat("media.amplayer.disable-aformat");
    if ((am_p->playctrl_info.no_audio_flag) ||  \
        ((1 << am_p->astream_info.audio_format)&filter_afmt)) {
        log_print("aformat type %d disable \n", am_p->astream_info.audio_format);
        am_p->astream_info.has_audio = 0;
        set_player_error_no(am_p, am_p->playctrl_info.no_audio_flag ? PLAYER_SET_NOAUDIO : PLAYER_UNSUPPORT_AUDIO);
        update_player_states(am_p, 1);
    } else if (!am_p->astream_info.has_audio) {
        if (am_p->vstream_info.has_video) {
            //log_print("[%s:%d]afmt=%d IS_AFMT_VALID(afmt)=%d\n", __FUNCTION__, __LINE__, am_p->astream_info.audio_format, IS_AFMT_VALID(am_p->astream_info.audio_format));
            if (IS_AFMT_VALID(am_p->astream_info.audio_format)) {
                set_player_error_no(am_p, PLAYER_UNSUPPORT_AUDIO);
                update_player_states(am_p, 1);
            } else {
                set_player_error_no(am_p, PLAYER_NO_AUDIO);
                update_player_states(am_p, 1);
            }
        } else {
            log_error("Can't support the file!\n");
            return PLAYER_UNSUPPORT;
        }
    }

    if ((am_p->stream_type == STREAM_ES) &&
        (am_p->vstream_info.video_format == VFORMAT_REAL)) {
        log_print("[%s:%d]real ES not support!\n", __FUNCTION__, __LINE__);
        return PLAYER_UNSUPPORT;
    }
#if 0
    if ((am_p->playctrl_info.no_audio_flag) ||
        ((!strcmp(dts_value, "true")) && (am_p->astream_info.audio_format == AFORMAT_DTS)) ||
        ((!strcmp(ac3_value, "true")) && (am_p->astream_info.audio_format == AFORMAT_AC3))) {
        am_p->astream_info.has_audio = 0;
    }
#endif
    if (!am_p->vstream_info.has_video) {
        am_p->vstream_num = 0;
    }

    if (!am_p->astream_info.has_audio) {
        am_p->astream_num = 0;
    }

    if ((!am_p->playctrl_info.has_sub_flag) && (!am_p->sstream_info.has_sub)) {
        am_p->sstream_num = 0;
    }

    am_p->sstream_info.has_sub &= am_p->playctrl_info.has_sub_flag;
    am_p->astream_info.resume_audio = am_p->astream_info.has_audio;
    am_p->sstream_info.has_sub = 0;
    if (am_p->vstream_info.has_video == 0) {
        am_p->playctrl_info.video_end_flag = 1;
    }
    if (am_p->astream_info.has_audio == 0) {
        am_p->playctrl_info.audio_end_flag = 1;
    }

    if (am_p->astream_info.has_audio) {

        if (!am_p->playctrl_info.raw_mode &&
            (am_p->astream_info.audio_format == AFORMAT_AAC || am_p->astream_info.audio_format == AFORMAT_AAC_LATM)) {
            ret = extract_adts_header_info(am_p);
            if (ret != PLAYER_SUCCESS) {
                log_error("[%s:%d]extract adts header failed! ret=0x%x\n", __FUNCTION__, __LINE__, -ret);
                return ret;
            }
        } else if (am_p->astream_info.audio_format == AFORMAT_COOK ||
                   am_p->astream_info.audio_format == AFORMAT_RAAC) {
            log_print("[%s:%d]get real auido header info...\n", __FUNCTION__, __LINE__);
            url_fseek(pb, 0, SEEK_SET); // get cook info from the begginning of the file
            buf = MALLOC(AUDIO_EXTRA_DATA_SIZE);
            if (buf) {
                do {
                    buf += total_rev_bytes;
                    rev_byte = get_buffer(pb, buf, (AUDIO_EXTRA_DATA_SIZE - total_rev_bytes));
                    log_print("[%s:%d]rev_byte=%d total=%d\n", __FUNCTION__, __LINE__, rev_byte, total_rev_bytes);
                    if (rev_byte < 0) {
                        if (rev_byte == AVERROR(EAGAIN)) {
                            continue;
                        } else {
                            log_error("[stream_rm_init]audio codec init faile--can't get real_cook decode info!\n");
                            return PLAYER_REAL_AUDIO_FAILED;
                        }
                    } else {
                        total_rev_bytes += rev_byte;
                        if (total_rev_bytes == AUDIO_EXTRA_DATA_SIZE) {
                            if (am_p->astream_info.extradata) {
                                FREE(am_p->astream_info.extradata);
                                am_p->astream_info.extradata = NULL;
                                am_p->astream_info.extradata_size = 0;
                            }
                            am_p->astream_info.extradata = buf;
                            am_p->astream_info.extradata_size = AUDIO_EXTRA_DATA_SIZE;
                            break;
                        } else if (total_rev_bytes > AUDIO_EXTRA_DATA_SIZE) {
                            log_error("[%s:%d]real cook info too much !\n", __FUNCTION__, __LINE__);
                            return PLAYER_FAILED;
                        }
                    }
                } while (1);
            } else {
                log_error("[%s:%d]no enough memory for real_cook_info\n", __FUNCTION__, __LINE__);
                return PLAYER_NOMEM;
            }
        }

    }

    if (am_p->vstream_info.has_video) {
        if (am_p->vstream_info.video_format == VFORMAT_MJPEG &&
            am_p->vstream_info.video_width >= 1280) {
            am_p->vstream_info.discard_pkt = 1;
            log_error("[%s:%d]HD mjmpeg, discard some vpkt, rate=%d\n", __FUNCTION__, __LINE__, am_p->vstream_info.video_rate);
            am_p->vstream_info.video_rate <<= 1;
            log_error("[%s:%d]HD mjmpeg, set vrate=%d\n", __FUNCTION__, __LINE__, am_p->vstream_info.video_rate);
        }
        if ((am_p->vstream_info.video_width == 1920) &&
            (am_p->vstream_info.video_height == 1088 || am_p->vstream_info.video_height == 1080) &&
            (am_p->vstream_info.video_rate <= (UNIT_FREQ / 50))) {
            set_poweron_clock_level(1);
        } else {
            set_poweron_clock_level(0);
        }
    }

    if (am_p->sstream_info.has_sub) {
        am_p->sstream_info.sub_has_found = 1;
    }
    return PLAYER_SUCCESS;
}

/*
 * player_startsync_set
 *
 * reset start sync using prop media.amplayer.startsync.mode
 * 0 none start sync
 * 1 slow sync repeate mode
 * 2 drop pcm mode
 *
 * */

static int player_startsync_set()
{
    const char * startsync_mode = "/sys/class/tsync/startsync_mode";
    const char * droppcm_prop = "sys.amplayer.drop_pcm"; // default enable
    const char * slowsync_path = "/sys/class/tsync/slowsync_enable";
    const char * slowsync_repeate_path = "/sys/class/video/slowsync_repeat_enable";
    char value[PROPERTY_VALUE_MAX];

    int mode = get_sysfs_int(startsync_mode);
    /*
        int ret = property_get(startsync_prop, value, NULL);
        if (ret <= 0) {
            log_print("start sync mode prop not setting ,using default none \n");
        }
        else
            mode = atoi(value);
    */
    log_print("start sync mode desp: 0 -none 1-slowsync repeate 2-droppcm \n");
    log_print("start sync mode = %d \n", mode);

    if (mode == 0) { // none case
        set_sysfs_int(slowsync_path, 0);
        //property_set(droppcm_prop, "0");
        set_sysfs_int(slowsync_repeate_path, 0);
    }

    if (mode == 1) { // slow sync repeat mode
        set_sysfs_int(slowsync_path, 1);
        //property_set(droppcm_prop, "0");
        set_sysfs_int(slowsync_repeate_path, 1);
    }

    if (mode == 2) { // drop pcm mode
        set_sysfs_int(slowsync_path, 0);
        //property_set(droppcm_prop, "1");
        set_sysfs_int(slowsync_repeate_path, 0);
    }

    return 0;
}

static int fb_reach_head(play_para_t *para)
{
    para->playctrl_info.time_point = 0;
    set_player_state(para, PLAYER_FB_END);
    update_playing_info(para);
    update_player_states(para, 1);
    return 0;
}

static int ff_reach_end(play_para_t *para)
{
    //set_black_policy(para->playctrl_info.black_out);
    para->playctrl_info.f_step = 0;
    if (para->playctrl_info.loop_flag) {
        para->playctrl_info.time_point = 0;
        para->playctrl_info.init_ff_fr = 0;
        log_print("ff reach end,loop play\n");
    } else {
        para->playctrl_info.time_point = para->state.full_time;
        para->playctrl_info.end_flag = 1;
        log_print("ff reach end,stop play\n");
    }
    set_player_state(para, PLAYER_FF_END);
    update_playing_info(para);
    update_player_states(para, 1);
    return 0;
}

static void player_ctrl_flag_reset(p_ctrl_info_t *cflag)
{
    cflag->video_end_flag = 0;
    cflag->audio_end_flag = 0;
    cflag->end_flag = 0;
    cflag->read_end_flag = 0;
    cflag->video_low_buffer = 0;
    cflag->audio_low_buffer = 0;
    cflag->audio_ready = 0;
    cflag->audio_switch_vmatch = 0;
    cflag->audio_switch_smatch = 0;
    //cflag->pause_flag = 0;
}

void player_clear_ctrl_flags(p_ctrl_info_t *cflag)
{
    cflag->fast_backward = 0;
    cflag->fast_forward = 0;
    cflag->search_flag = 0;
    cflag->reset_flag = 0;
    cflag->f_step = 0;
    cflag->hls_forward = 0;
    cflag->hls_backward= 0;
}

void player_para_reset(play_para_t *para)
{
    player_ctrl_flag_reset(&para->playctrl_info);
    if (!url_support_time_seek(para->pFormatCtx->pb)) {
        para->discontinue_point = 0;
    }
    para->discontinue_flag = 0;
    para->discontinue_point = 0;
    //para->playctrl_info.pts_valid = 0;
    para->playctrl_info.check_audio_ready_ms = 0;
    para->playctrl_info.write_end_header_flag = 0;
    para->playctrl_info.trick_wait_flag = 0;
    if (get_player_state(para) == PLAYER_BUFFERING) {
        para->playctrl_info.pause_flag = 0;
    }
    MEMSET(&para->write_size, 0, sizeof(read_write_size));
    MEMSET(&para->read_size, 0, sizeof(read_write_size));
}
int player_switch_para(play_para_t *p_para)
{
    int video_index = -1;
    int audio_index = -1;
    int sub_index = -1;
    AVStream *pStream;
    AVCodecContext  *pCodecCtx;
    AVFormatContext *pFormatCtx = p_para->pFormatCtx;
    mstream_info_t* new_stream_info = &p_para->media_info.ts_programe_info.new_stream_info;
    if (p_para->playctrl_info.switch_ts_program_flag == 1&&p_para->playctrl_info.switch_param_flag){
        log_print("[%s %d] switch to video_index:%d audio_index:%d\n", __FUNCTION__, __LINE__, new_stream_info->cur_video_index, new_stream_info->cur_audio_index);
        if (new_stream_info->cur_video_index < pFormatCtx->nb_streams)
            p_para->vstream_info.video_index = new_stream_info->cur_video_index ;
        else
            log_print("[%s %d] switch to video_index:%d is error\n", __FUNCTION__, __LINE__,new_stream_info->cur_video_index);


        if (new_stream_info->cur_audio_index  < pFormatCtx->nb_streams)
            p_para->astream_info.audio_index = new_stream_info->cur_audio_index ;
        else
            log_print("[%s %d] switch to audio_index:%d is error\n", __FUNCTION__, __LINE__,new_stream_info->cur_audio_index);

        //get_av_codec_type(get_av_codec_type);
        video_index = p_para->vstream_info.video_index;
        audio_index = p_para->astream_info.audio_index;
        sub_index = p_para->sstream_info.sub_index;
        log_print("[%s:%d]vidx=%d aidx=%d sidx=%d\n", __FUNCTION__, __LINE__, video_index, audio_index, sub_index);

    if (video_index != -1) {
        pStream = pFormatCtx->streams[video_index];
        pCodecCtx = pStream->codec;
        p_para->vstream_info.video_format   = video_type_convert(pCodecCtx->codec_id, 0);
        if (p_para->stream_type == STREAM_ES && pCodecCtx->codec_tag != 0) {
            p_para->vstream_info.video_codec_type = video_codec_type_convert(pCodecCtx->codec_tag);
        } else {
            p_para->vstream_info.video_codec_type = video_codec_type_convert(pCodecCtx->codec_id);
        }
    }else {
        p_para->vstream_info.has_video = 0;
        log_print("no video specified!\n");
    }
    if (audio_index != -1) {
        get_audio_type(p_para);
    } else {
        p_para->astream_info.has_audio = 0;
        log_print("no audio specified!\n");
    }
    if (sub_index != -1) {
        pStream = pFormatCtx->streams[sub_index];
        p_para->sstream_info.sub_pid = (unsigned short)pStream->id;
        p_para->sstream_info.sub_type = pStream->codec->codec_id;
    } else {
        p_para->sstream_info.has_sub = 0;
    }
    }
    return 0;
}
int player_dec_reset(play_para_t *p_para)
{
    const stream_decoder_t *decoder;
    int ret = PLAYER_SUCCESS;
    AVFormatContext *pFormatCtx = p_para->pFormatCtx;;
    float time_point = p_para->playctrl_info.time_point;
    int64_t timestamp = 0;
    int mute_flag = 0;
    int seek_retry_count = am_getconfig_int_def("media.amplayer.seek_retry", 0);
    int video_index, audio_index;

    if ((p_para->playctrl_info.switch_ts_program_flag == 1)&&(!p_para->playctrl_info.switch_param_flag)) {
        log_print("[%s %d] video_index:%d audio_index:%d total:%d\n", __FUNCTION__, __LINE__, video_index, audio_index, pFormatCtx->nb_streams);

        video_index = player_get_ts_index_of_pid(p_para, p_para->playctrl_info.switch_ts_video_pid);
        audio_index = player_get_ts_index_of_pid(p_para, p_para->playctrl_info.switch_ts_audio_pid);

        log_print("[%s %d] switch to video_index:%d audio_index:%d\n", __FUNCTION__, __LINE__, video_index, audio_index);

        if (video_index < pFormatCtx->nb_streams)
            p_para->vstream_info.video_index = video_index;
        else
            video_index = p_para->vstream_info.video_index;

        if (audio_index < pFormatCtx->nb_streams)
            p_para->astream_info.audio_index = audio_index;
        else
            audio_index = p_para->astream_info.audio_index;

        p_para->playctrl_info.switch_ts_video_pid = 0;
        p_para->playctrl_info.switch_ts_audio_pid = 0;
    }

    player_startsync_set(); // maybe reset
    timestamp = (int64_t)(time_point * AV_TIME_BASE);
    if (p_para->vstream_info.has_video
        && (timestamp != pFormatCtx->start_time)
        && (p_para->stream_type == STREAM_ES)) {
        if (p_para->astream_info.has_audio && p_para->acodec) {
            codec_audio_automute(p_para->acodec->adec_priv, 1);
            mute_flag = 1;
        }

        if (p_para->player_need_reset == 1){
            if (p_para->acodec) {
                log_print("acodec reset");
                codec_set_dec_reset(p_para->acodec);
            }
        }

        if (p_para->vcodec) {
            codec_set_dec_reset(p_para->vcodec);
        }
    }

    decoder = p_para->decoder;
    if (decoder == NULL) {
        log_error("[player_dec_reset:%d]decoder null!\n", __LINE__);
        return PLAYER_NO_DECODER;
    }

    if (decoder->release(p_para) != PLAYER_SUCCESS) {
        log_error("[player_dec_reset] deocder release failed!\n");
        return DECODER_RESET_FAILED;
    }
    /*make sure have enabled.*/
    if (p_para->astream_info.has_audio && p_para->vstream_info.has_video) {
        set_tsync_enable(1);

        p_para->playctrl_info.avsync_enable = 1;
    } else {
        set_tsync_enable(0);
        p_para->playctrl_info.avsync_enable = 0;
    }
    if (decoder->init(p_para) != PLAYER_SUCCESS) {
        log_print("[player_dec_reset] deocder init failed!\n");
        return DECODER_RESET_FAILED;
    }

    if (p_para->astream_info.has_audio && p_para->acodec) {
        p_para->codec = p_para->acodec;
        if (p_para->vcodec) {
            p_para->codec->has_video = 1;
        }
        log_print("[%s:%d]para->codec pointer to acodec!\n", __FUNCTION__, __LINE__);
    } else if (p_para->vcodec) {
        p_para->codec = p_para->vcodec;
        log_print("[%s:%d]para->codec pointer to vcodec!\n", __FUNCTION__, __LINE__);
    }

    if (p_para->playctrl_info.fast_forward) {
        if (p_para->playctrl_info.time_point >= p_para->state.full_time &&
            p_para->state.full_time > 0) {
            p_para->playctrl_info.end_flag = 1;
            set_black_policy(p_para->playctrl_info.black_out);
            log_print("[%s]ff end: tpos=%d black=%d\n", __FUNCTION__, p_para->playctrl_info.time_point, p_para->playctrl_info.black_out);
            return ret;
        }

        log_print("[player_dec_reset:%d]time_point=%f step=%d\n", __LINE__, p_para->playctrl_info.time_point, p_para->playctrl_info.f_step);
		if(p_para->playctrl_info.fast_forward)
        	p_para->playctrl_info.time_point += p_para->playctrl_info.f_step;
        if (p_para->playctrl_info.time_point >= p_para->state.full_time &&
            p_para->state.full_time > 0) {
            ff_reach_end(p_para);
            log_print("reach stream end,play end!\n");
            if((p_para->pFormatCtx) && (p_para->pFormatCtx->pb) && 
                 (p_para->pFormatCtx->pb->local_playback == 0))
                 send_event(p_para, PLAYER_EVENTS_FF_END, 0,"FF end");
        }
    } else if (p_para->playctrl_info.fast_backward) {
        if (p_para->playctrl_info.time_point == 0) {
            p_para->playctrl_info.init_ff_fr = 0;
            p_para->playctrl_info.f_step = 0;
        }
        if ((p_para->playctrl_info.time_point >= p_para->playctrl_info.f_step) &&
            (p_para->playctrl_info.time_point > 0)) {
			if(p_para->playctrl_info.fast_backward)
            	p_para->playctrl_info.time_point -= p_para->playctrl_info.f_step;
        } else {
            fb_reach_head(p_para);
            log_print("reach stream head,fast backward stop,play from start!\n");
            send_event(p_para, PLAYER_EVENTS_FB_END, 0,"FB end");
#if 0
            if((p_para->pFormatCtx) && (p_para->pFormatCtx->pb) && 
				(p_para->pFormatCtx->pb->local_playback == 0))
				send_event(p_para, PLAYER_EVENTS_FB_END, 0,"FB end");
#endif
        }
    } else {
        if (!p_para->playctrl_info.search_flag && p_para->playctrl_info.loop_flag) {
            p_para->playctrl_info.time_point = 0;
        }
    }
    if (p_para->stream_type == STREAM_AUDIO) {
        p_para->astream_info.check_first_pts = 0;
    }

    /* set disable slow sync */
    if (p_para->vstream_info.has_video) {
        int disable_slowsync = am_getconfig_bool("libplayer.slowsync.disable");
        if ((p_para->pFormatCtx->pb != NULL && p_para->pFormatCtx->pb->is_slowmedia) || (p_para->playctrl_info.f_step == 0)) {
            disable_slowsync = 0;
        }

        if (p_para->vcodec != NULL) {
            codec_disalbe_slowsync(p_para->vcodec, disable_slowsync);
        } else if (p_para->codec != NULL) {
            codec_disalbe_slowsync(p_para->codec, disable_slowsync);
        }
    }
        
    log_print("player dec reset p_para->playctrl_info.time_point=%f\n", p_para->playctrl_info.time_point);
    if ((p_para->playctrl_info.time_point >= 0) && 
		((p_para->state.full_time > 0) || 
		 (p_para->start_param->is_livemode == 1))&& 
		(p_para->playctrl_info.hls_forward == 0) &&
		(p_para->playctrl_info.hls_backward == 0)){
		if (p_para->playctrl_info.cache_enable == 0) {
			player_mate_wake(p_para, 100 * 1000);
		}

RETRY:    
        ret = time_search(p_para, -1);
        if(ret != PLAYER_SUCCESS && url_interrupt_cb() == 0 && seek_retry_count-- > 0){
            usleep(100*1000);
            goto RETRY;
        }
        // in case of seeking failed caused player exit, return ok when network down
        if (ret != PLAYER_SUCCESS && am_getconfig_int_def("net.ethwifi.up",3) == 0) {
            log_print("network down, return ok \n");
            ret = PLAYER_SUCCESS;
        }

		if (p_para->playctrl_info.cache_enable == 0) {
			player_mate_sleep(p_para);
		}
    } else {
        if (p_para->pFormatCtx && p_para->pFormatCtx->pb && p_para->stream_type == STREAM_RM) {
            int errorretry = 100;
            AVPacket pkt;
            log_print("do real read seek to next frame...\n");
            av_read_frame_flush(p_para->pFormatCtx);
            ret = av_read_frame(p_para->pFormatCtx, &pkt);
            do {
                ret = av_read_frame(p_para->pFormatCtx, &pkt);/*read utils to good pkt*/
                if (ret >= 0) {
                    break;
                }
            } while (errorretry-- > 0);
            if (errorretry <= 0 && ret < 0) {
                log_print("NOT find a good frame .....\n");
            }
            if (!ret) {
                if (pkt.pts > 0 && pkt.pos > 0) {
                    log_print("read a good frame  t=%lld.....\n", pkt.pts);
                    AVStream *st = p_para->pFormatCtx->streams[pkt.stream_index];
                    int64_t t = av_rescale(pkt.pts, AV_TIME_BASE * st->time_base.num, (int64_t)st->time_base.den);
                    if (st->start_time != (int64_t)AV_NOPTS_VALUE) {
                        t -= st->start_time;
                    }
                    if (t < 0) {
                        t = 0;
                    }
                    log_print("read a good frame changedd  t=%lld..and seek to next key frame...\n", t);
                    av_seek_frame(p_para->pFormatCtx, p_para->vstream_info.video_index, t, 0); /*seek to next KEY frame*/
                    p_para->playctrl_info.time_point = (float)t / AV_TIME_BASE;
                }
                av_free_packet(&pkt);
            }
        }
        if (p_para->playctrl_info.reset_drop_buffered_data && /*drop data for less delay*/
            p_para->stream_type == STREAM_TS &&
            p_para->pFormatCtx->pb) {
#define S_TOPBUF_LEN (188*10*8)
#define S_ONCE_READ_L (188*8)
            char readbuf[S_ONCE_READ_L];
            int ret = S_ONCE_READ_L;
            int maxneeddroped = S_TOPBUF_LEN;
            int totaldroped = 0;
            avio_reset(p_para->pFormatCtx->pb, 0); /*clear ffmpeg's  buffers data.*/
            while (ret == S_ONCE_READ_L && maxneeddroped > 0) { /*do read till read max,or top buffer underflow to droped steamsource buffers data*/
                ret = get_buffer(p_para->pFormatCtx->pb, readbuf, S_ONCE_READ_L);
                maxneeddroped -= ret;
                if (ret > 0) {
                    totaldroped += ret;
                }
            }
            log_print("reset total droped data len=%d\n", totaldroped);
        }

        if (p_para->stream_type == STREAM_TS && p_para->vstream_info.has_video &&
            (VFORMAT_MPEG12 == p_para->vstream_info.video_format
            || VFORMAT_H264 == p_para->vstream_info.video_format
            || VFORMAT_AVS == p_para->vstream_info.video_format
            || (VFORMAT_HEVC == p_para->vstream_info.video_format &&
                p_para->pFormatCtx->pb->local_playback == 1))) {
            p_para->playctrl_info.seek_keyframe = 1;
        }

        p_para->playctrl_info.reset_drop_buffered_data = 0;
        ret = PLAYER_SUCCESS;/*do reset only*/
    }
    if (ret != PLAYER_SUCCESS) {
        log_error("[player_dec_reset]time search failed !ret = -%x\n", -ret);
    } else {
        /*clear the maybe end flags*/
        p_para->playctrl_info.audio_end_flag = 0;
        p_para->playctrl_info.video_end_flag = 0;
        p_para->playctrl_info.read_end_flag = 0;
        p_para->playctrl_info.video_low_buffer = 0;
        p_para->playctrl_info.audio_low_buffer = 0;
    }

    if (mute_flag) {
        log_print("[%s:%d]audio_mute=%d\n", __FUNCTION__, __LINE__, p_para->playctrl_info.audio_mute);
        codec_audio_automute(p_para->acodec->adec_priv, p_para->playctrl_info.audio_mute);
    }
    if(p_para->playctrl_info.switch_param_flag)
        p_para->playctrl_info.switch_param_flag = 0;
    p_para->play_last_reset_systemtime_us = player_get_systemtime_ms();
    p_para->last_buffering_av_delay_ms = 0;
    p_para->latest_lowlevel_av_delay_ms = -1;
    return ret;
}
static int check_ctx_bitrate(play_para_t *p_para)
{
    AVFormatContext *ic = p_para->pFormatCtx;
    AVStream *st;
    int bit_rate = 0;
    unsigned int i;
    int flag = 0;
    for (i = 0; i < ic->nb_streams; i++) {
        st = ic->streams[i];
        if (p_para->file_type == RM_FILE) {
            if (st->codec->codec_type == CODEC_TYPE_VIDEO ||
                st->codec->codec_type == CODEC_TYPE_AUDIO) {
                bit_rate += st->codec->bit_rate;
            }
        } else {
            bit_rate += st->codec->bit_rate;
        }
        if (st->codec->codec_type == CODEC_TYPE_VIDEO && st->codec->bit_rate == 0) {
            flag = -1;
        }
        if (st->codec->codec_type == CODEC_TYPE_AUDIO && st->codec->bit_rate == 0) {
            flag = -1;
        }
    }
    log_print("[check_ctx_bitrate:%d]bit_rate=%d ic->bit_rate=%d\n", __LINE__, bit_rate, ic->bit_rate);
    if (p_para->file_type == ASF_FILE) {
        if (ic->bit_rate == 0) {
            ic->bit_rate = bit_rate;
        }
    } else {
        if (bit_rate > ic->bit_rate || (ic->bit_rate - bit_rate) > 1000000000) {
            ic->bit_rate = bit_rate ;
        }
    }
    log_print("[check_ctx_bitrate:%d]bit_rate=%d ic->bit_rate=%d\n", __LINE__, bit_rate, ic->bit_rate);
    return flag;
}

static void subtitle_para_init(play_para_t *player)
{
    AVFormatContext *pCtx = player->pFormatCtx;
    int frame_rate_num, frame_rate_den;
    float video_fps;
    char out[20];
    char default_sub = "firstindex";


    if (player->vstream_info.has_video) {
        video_fps = (UNIT_FREQ) / (float)player->vstream_info.video_rate;
        set_subtitle_fps(video_fps * 100);
    }

    if (!player->sstream_info.has_sub) {
        return ;    /*no subtitle ignore init for fast start playing*/
    }

    set_subtitle_num(player->sstream_num);
    set_subtitle_curr(0);
    set_subtitle_index(0);

    //FFT: get proerty from build.prop
    GetSystemSettingString("media.amplayer.divx.certified", out, &default_sub);
    log_print("[%s:%d]out = %s !\n", __FUNCTION__, __LINE__, out);

    //FFT: set default subtitle index for divx certified
    if (strcmp(out, "enable") == 0) {
        set_subtitle_enable(0);
        set_subtitle_curr(0xff);
        log_print("[%s:%d]set default subtitle index !\n", __FUNCTION__, __LINE__);
    }
    if (player->sstream_info.has_sub) {
        if (player->sstream_info.sub_type == CODEC_ID_DVD_SUBTITLE) {
            set_subtitle_subtype(0);
        } else if (player->sstream_info.sub_type == CODEC_ID_HDMV_PGS_SUBTITLE) {
            set_subtitle_subtype(1);
        } else if (player->sstream_info.sub_type == CODEC_ID_XSUB) {
            set_subtitle_subtype(2);
        } else if (player->sstream_info.sub_type == CODEC_ID_TEXT || \
                   player->sstream_info.sub_type == CODEC_ID_SSA) {
            set_subtitle_subtype(3);
        } else if (player->sstream_info.sub_type == CODEC_ID_DVB_SUBTITLE) {
            set_subtitle_subtype(5);
        } else {
            set_subtitle_subtype(4);
        }
    } else {
        set_subtitle_subtype(0);
    }
    if (player->astream_info.start_time != -1) {
        set_subtitle_startpts(player->astream_info.start_time);
        log_print("player set startpts is 0x%llx\n", player->astream_info.start_time);
    } else if (player->vstream_info.start_time != -1) {
        set_subtitle_startpts(player->vstream_info.start_time);
        log_print("player set startpts is 0x%llx\n", player->vstream_info.start_time);
    } else {
        set_subtitle_startpts(0);
    }
}

///////////////////////////////////////////////////////////////////
static void init_es_sub(play_para_t *p_para)
{
    int i;
    int subnum = p_para->sstream_num;

    for (i = 0; i <= subnum; i++) {
        es_sub_buf[i].subid = i;
        es_sub_buf[i].rdp = 0;
        es_sub_buf[i].wrp = 0;
        es_sub_buf[i].size = 0;
        p_para->sstream_info.sub_buf[i] = (char *)malloc(SUBTITLE_SIZE * sizeof(char));
        if (p_para->sstream_info.sub_buf[i] == NULL) {
            log_print("## [%s:%d] malloc subbuf i=%d, failed! ---------\n", __FUNCTION__, __LINE__, i);
            p_para->sstream_info.has_sub = 0;
        }
        es_sub_buf[i].sub_buf = &(p_para->sstream_info.sub_buf[i][0]);
        memset(&(p_para->sstream_info.sub_buf[i][0]), 0, SUBTITLE_SIZE);
    }
    p_para->sstream_info.sub_stream = 0;
}
static void set_es_sub(play_para_t *p_para)
{
    int i;
    AVFormatContext *pFormat = p_para->pFormatCtx;
    AVStream *pStream;
    AVCodecContext *pCodec;
    int sub_index = 0;

    if (p_para->sstream_info.has_sub == 0) {
        return ;
    }

    for (i = 0; i < pFormat->nb_streams; i++) {
        pStream = pFormat->streams[i];
        pCodec = pStream->codec;
        if (pCodec->codec_type == CODEC_TYPE_SUBTITLE) {
            es_sub_buf[sub_index].subid = pStream->id;
            p_para->sstream_info.sub_stream |= 1 << pStream->index;
            log_print("[%s:%d]es_sub_buf[sub_index].i=%d,subid = %d, sub_index =%d pStream->id=%d, sub_stream=0x%x,!\n", __FUNCTION__, __LINE__, i, es_sub_buf[sub_index].subid, sub_index, pStream->id, p_para->sstream_info.sub_stream);
            sub_index++;
        }
    }
}


int player_dec_init(play_para_t *p_para)
{
    pfile_type file_type = UNKNOWN_FILE;
    pstream_type stream_type = STREAM_UNKNOWN;
    int ret = 0;
    int full_time = 0;
    int full_time_ms = 0;
    int i;
    int wvenable = 0;
    AVStream *st;
    int64_t streamtype = -1;

    player_startsync_set();
    ret = ffmpeg_parse_file(p_para);
    if (ret != FFMPEG_SUCCESS) {
        log_print("[player_dec_init]ffmpeg_parse_file failed(%s)*****ret=%x!\n", p_para->file_name, ret);
        return ret;
    }
    dump_format(p_para->pFormatCtx, 0, p_para->file_name, 0);

    int t, is_hevc = 0;
    int is_truehd = 0;
    for (t = 0; t < p_para->pFormatCtx->nb_streams; t++) {
        if (p_para->pFormatCtx->streams[t]->codec->codec_id == CODEC_ID_HEVC) {
            is_hevc = 1;
            break;
        }
    }
    //for ts file, ac3 & truehd share the same pid,our hw demux mix the two audio streams together which cause decoder pause
    for (t = 0; t < p_para->pFormatCtx->nb_streams; t++) {
        if (p_para->pFormatCtx->streams[t]->codec->codec_id == CODEC_ID_TRUEHD) {
            is_truehd = 1;
            break;
        }
    }
    ret = set_file_type(p_para->pFormatCtx->iformat->name, &file_type, &stream_type);
    if ((p_para->pFormatCtx && p_para->pFormatCtx->pb && p_para->pFormatCtx->pb->isprtvp)) {
        p_para->pFormatCtx->flags |= AVFMT_FLAG_PR_TVP;
        log_print("PlayReady ts  TVP, need use hardware demux\n");
    }
    if ((memcmp(p_para->pFormatCtx->iformat->name, "mpegts", 6) == 0) && ((p_para->pFormatCtx->flags & AVFMT_FLAG_PR_TVP) == 0)) {
        if (p_para->start_param->is_ts_soft_demux || is_hevc == 1 || is_truehd == 1) {
            log_print("Player config used soft demux,used soft demux now.\n");
            file_type = STREAM_FILE;
            stream_type = STREAM_ES;
            ret = PLAYER_SUCCESS;
        } else if (am_getconfig_bool_def("libplayer.ts.softdemux", 1)) {
            log_print("configned all ts streaming used soft demux,used soft demux now.\n");
            file_type = STREAM_FILE;
            stream_type = STREAM_ES;
            ret = PLAYER_SUCCESS;
        } else if (p_para->pFormatCtx->pb && p_para->pFormatCtx->pb->is_slowmedia &&  //is network...
                   am_getconfig_bool_def("libplayer.netts.softdemux", 1)) {
            log_print("configned network tsstreaming used soft demux,used soft demux now.\n");
            file_type = STREAM_FILE;
            stream_type = STREAM_ES;
            ret = PLAYER_SUCCESS;
        } else if (am_getconfig_bool("libplayer.livets.softdemux")) {
            avio_getinfo(p_para->pFormatCtx->pb, AVCMD_HLS_STREAMTYPE, 0, &streamtype);
            log_print("livingstream [%d]\n", streamtype);
            if (p_para->pFormatCtx->pb && p_para->pFormatCtx->pb->is_slowmedia &&  //is network...
                (streamtype == 0)) {
                log_print("livingstream configned network tsstreaming used soft demux,used soft demux now.\n");
                file_type = STREAM_FILE;
                stream_type = STREAM_ES;
                ret = PLAYER_SUCCESS;
            }
        }

        if (p_para->playctrl_info.lowbuffermode_flag || am_getconfig_bool("media.libplayer.wfd")/*||
            av_strstart(p_para->file_name, "udp://", NULL) || av_strstart(p_para->file_name, "rtsp://", NULL)*/) {
            if (!p_para->start_param->is_ts_soft_demux && stream_type != STREAM_TS && 
                !am_getconfig_bool("libplayer.broadcast.debug")) {
                log_print("Player reconfig use hwdemux for wfd now\n");
                file_type = MPEG_FILE;
                stream_type = STREAM_TS;
                ret = PLAYER_SUCCESS;
            }
        }
    }else if ((memcmp(p_para->pFormatCtx->iformat->name, "mpeg", 4) == 0) && ((p_para->pFormatCtx->flags & AVFMT_FLAG_PR_TVP) == 0)) {
        if (am_getconfig_bool("libplayer.ps.softdemux")) {
            log_print("configned all ps streaming used soft demux,used soft demux now.\n");
            file_type = STREAM_FILE;
            stream_type = STREAM_ES;
            ret = PLAYER_SUCCESS;
        }
    }


    if (ret != PLAYER_SUCCESS) {
        set_player_state(p_para, PLAYER_ERROR);
        p_para->state.status = PLAYER_ERROR;
        log_print("[player_dec_init]set_file_type failed!\n");
        goto init_fail;
    }

    if (STREAM_ES == stream_type) {
        p_para->playctrl_info.raw_mode = 0;
    } else {
        p_para->playctrl_info.raw_mode = 1;
    }

    p_para->file_size = p_para->pFormatCtx->file_size;
    if (p_para->file_size < 0) {
        p_para->pFormatCtx->valid_offset = INT64_MAX;
    }

    if (p_para->pFormatCtx->duration != -1) {
        p_para->state.full_time = p_para->pFormatCtx->duration / AV_TIME_BASE;
        p_para->state.full_time_ms = (p_para->pFormatCtx->duration) / (AV_TIME_BASE / 1000);
    } else {
        p_para->state.full_time = -1;
        p_para->state.full_time_ms = -1;
    }

    p_para->state.name = p_para->file_name;
    p_para->file_type = file_type;
    p_para->stream_type = stream_type;
    log_print("[player_dec_init:%d]fsize=%lld full_time=%d bitrate=%d\n", __LINE__, p_para->file_size, p_para->state.full_time, p_para->pFormatCtx->bit_rate);

    if (p_para->stream_type == STREAM_AUDIO) {
        p_para->astream_num = 1;
    } else if (p_para->stream_type == STREAM_VIDEO) {
        p_para->vstream_num = 1;
    }

    ret = set_decode_para(p_para);
    if (ret != PLAYER_SUCCESS) {
        log_error("set_decode_para failed, ret = -0x%x\n", -ret);
        goto init_fail;
    }

    if (p_para->sstream_info.has_sub) {
        if (!am_getconfig_bool("media.amplayer.sublowmem")) {
            init_es_sub(p_para);
            set_es_sub(p_para);
        }
    }
#ifdef DUMP_INDEX
    int i, j;
    AVStream *pStream;
    log_print("*********************************************\n");
    for (i = 0; i < p_para->pFormatCtx->nb_streams; i ++) {
        pStream = p_para->pFormatCtx->streams[2];
        for (j = 0; j < pStream->nb_index_entries; j++) {
            log_print("stream[%d]:idx[%d] pos:%llx time:%llx\n", 2, j, pStream->index_entries[j].pos, pStream->index_entries[j].timestamp);
        }
    }
    log_print("*********************************************\n");
#endif

    if (memcmp(p_para->pFormatCtx->iformat->name, "mpegts", 6) && p_para->stream_type != STREAM_TS && p_para->stream_type != STREAM_PS) {
        if (check_ctx_bitrate(p_para) == 0) {
            if ((0 != p_para->pFormatCtx->bit_rate) && (0 != p_para->file_size)) {
                full_time = (int)((p_para->file_size << 3) / p_para->pFormatCtx->bit_rate);
                full_time_ms = (int)(((p_para->file_size << 3) * 1000) / p_para->pFormatCtx->bit_rate);
                log_print("[player_dec_init:%d]bit_rate=%d file_size=%lld full_time=%d\n", __LINE__, p_para->pFormatCtx->bit_rate, p_para->file_size, full_time);

                if (p_para->state.full_time - full_time > 1200) {
                    p_para->state.full_time = full_time;
                    p_para->state.full_time_ms = full_time_ms;
                }
            }
        }
    }

    if (p_para->state.full_time <= 0) {
        if (p_para->stream_type == STREAM_PS || p_para->stream_type == STREAM_TS) {
            check_ctx_bitrate(p_para);
            if ((0 != p_para->pFormatCtx->bit_rate) && (0 != p_para->file_size)) {
                p_para->state.full_time = (int)((p_para->file_size << 3) / p_para->pFormatCtx->bit_rate);
                p_para->state.full_time_ms = (int)(((p_para->file_size << 3) * 1000) / p_para->pFormatCtx->bit_rate);
            } else {
                p_para->state.full_time = -1;
                p_para->state.full_time_ms = -1;
            }
        } else {
            p_para->state.full_time = -1;
            p_para->state.full_time_ms = -1;
        }
        if (p_para->state.full_time == -1) {
            if (p_para->pFormatCtx->pb) {
                int duration = url_ffulltime(p_para->pFormatCtx->pb);
                if (duration > 0) {
                    p_para->state.full_time = duration;
                }
            }
        }

    }

    //update full time with duration parsed from url
    if (p_para->playctrl_info.duration_url > 0) {
        //choose smaller duration
        if (p_para->state.full_time > p_para->playctrl_info.duration_url / 1000 || p_para->pFormatCtx->duration == AV_NOPTS_VALUE) {
            p_para->state.full_time = p_para->playctrl_info.duration_url / 1000;
            p_para->state.full_time_ms = p_para->playctrl_info.duration_url;
        }
    }

    log_print("[player_dec_init:%d]bit_rate=%d file_size=%lld file_type=%d stream_type=%d full_time=%d\n", __LINE__, p_para->pFormatCtx->bit_rate, p_para->file_size, p_para->file_type, p_para->stream_type, p_para->state.full_time);

    if (p_para->pFormatCtx->iformat->flags & AVFMT_NOFILE) {
        p_para->playctrl_info.raw_mode = 0;
    }
    if (p_para->playctrl_info.raw_mode) {
        if (p_para->pFormatCtx->bit_rate > 0) {
            p_para->max_raw_size = (p_para->pFormatCtx->bit_rate >> 3) >> 4 ; //KB/s /16
            if (p_para->max_raw_size < MIN_RAW_DATA_SIZE) {
                p_para->max_raw_size = MIN_RAW_DATA_SIZE;
            }
            if (p_para->max_raw_size > MAX_RAW_DATA_SIZE) {
                p_para->max_raw_size = MAX_RAW_DATA_SIZE;
            }
        } else {
            p_para->max_raw_size = MAX_BURST_WRITE;
        }
        if (p_para->pFormatCtx && p_para->pFormatCtx->pb &&
            p_para->pFormatCtx->pb->isprtvp) {
            p_para->max_raw_size = PR_BURST_READ_SIZE;
        }
        log_print("====bitrate=%d max_raw_size=%d\n", p_para->pFormatCtx->bit_rate, p_para->max_raw_size);
    }
    subtitle_para_init(p_para);
    p_para->latest_lowlevel_av_delay_ms = -1;
    p_para->last_buffering_av_delay_ms = 0;
    //set_tsync_enable(1);        //open av sync
    //p_para->playctrl_info.avsync_enable = 1;
    return PLAYER_SUCCESS;

init_fail:
    log_print("[player_dec_init]failed, ret=%x\n", ret);
    return ret;
}

int player_offset_init(play_para_t *p_para)
{
    int ret = PLAYER_SUCCESS;
    if(!p_para->off_init){
        if (p_para->playctrl_info.time_point >= 0) {
            log_print("player_offset_init time_search %f\n",p_para->playctrl_info.time_point);
            if (p_para->playctrl_info.time_point > 0) {
                if (p_para->vstream_info.has_video == 0 &&
                    p_para->astream_info.has_audio == 1) {
                    log_print("pure audio seek use flag any\n");
                    ret = time_search(p_para, AVSEEK_FLAG_ANY);
                } else
                ret = time_search(p_para, AVSEEK_FLAG_BACKWARD);
            } else {
                ret = time_search(p_para, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);    /*if seek to 0,don't care whether keyframe. */
            }
            set_player_state(p_para, PLAYER_SEARCHOK);
            update_player_states(p_para, 1);

            if (ret != PLAYER_SUCCESS) {
                set_player_state(p_para, PLAYER_ERROR);
                ret = PLAYER_SEEK_FAILED;
                log_error("[%s:%d]time_search to pos:%ds failed!", __FUNCTION__, __LINE__, p_para->playctrl_info.time_point);
                goto init_fail;
            }
            if (p_para->playctrl_info.time_point < p_para->state.full_time) {
                p_para->state.current_time = p_para->playctrl_info.time_point;
                p_para->state.current_ms = p_para->playctrl_info.time_point * 1000;
            }
            p_para->off_init = 1;
        } else if (p_para->playctrl_info.raw_mode) {
            log_print("*****data offset 0x%x\n", p_para->data_offset);
            url_fseek(p_para->pFormatCtx->pb, p_para->data_offset, SEEK_SET);
               p_para->off_init = 1;
        }

    }
    return PLAYER_SUCCESS;

init_fail:
    return ret;
}

int player_decoder_init(play_para_t *p_para)
{
    int ret;
    const stream_decoder_t *decoder = NULL;
    // kernel now set first_pcr = first_vpts - 10s
    // will trigger discontinue cause first video display anyway
    // here set default av_threshold_min to 11s
    set_sysfs_int("/sys/class/tsync/av_threshold_min", 90000 * am_getconfig_int_def("media.amplayer.discon_def", 4));
    if (!p_para->astream_info.has_audio && p_para->vstream_info.has_video) {
        set_sysfs_int("/sys/class/tsync/av_threshold_min", 90000);
        if ((p_para->pFormatCtx) && (p_para->pFormatCtx->pb) && (p_para->pFormatCtx->pb->local_playback == 1))
            set_sysfs_int("/sys/class/video/show_first_frame_nosync", 0);
    }

    decoder = find_stream_decoder(p_para->stream_type);
    if (decoder == NULL) {
        log_print("[player_dec_init]can't find decoder!\n");
        ret = PLAYER_NO_DECODER;
        goto failed;
    }

    if (p_para->astream_info.has_audio && p_para->vstream_info.has_video) {
        set_tsync_enable(1);

        p_para->playctrl_info.avsync_enable = 1;
    } else {
        set_tsync_enable(0);
        p_para->playctrl_info.avsync_enable = 0;
    }
    if (p_para->vstream_info.has_video) {
        /*
        if we have video,we need to clear the pcrsrc to 0.
        if not the pcrscr maybe a big number..
        */
        set_sysfs_str("/sys/class/tsync/pts_pcrscr", "0x0");
    }

    if (decoder->init(p_para) != PLAYER_SUCCESS) {
        log_print("[player_dec_init] codec init failed!\n");
        ret = DECODER_INIT_FAILED;
        goto failed;
    } else {
        set_stb_source_hiu();
        set_stb_demux_source_hiu();
    }

    p_para->decoder = decoder;
    p_para->check_end.end_count = CHECK_END_COUNT;
    p_para->check_end.interval = CHECK_END_INTERVAL;
    p_para->abuffer.check_rp_change_cnt = CHECK_AUDIO_HALT_CNT;
    p_para->vbuffer.check_rp_change_cnt = CHECK_VIDEO_HALT_CNT;

    if (p_para->astream_info.has_audio && p_para->acodec) {
        p_para->codec = p_para->acodec;
        if (p_para->vcodec) {
            p_para->codec->has_video = 1;
        }
        log_print("[%s:%d]para->codec pointer to acodec!\n", __FUNCTION__, __LINE__);
    } else if (p_para->vcodec) {
        p_para->codec = p_para->vcodec;
        log_print("[%s:%d]para->codec pointer to vcodec!\n", __FUNCTION__, __LINE__);
    }

    if (p_para->playctrl_info.lowbuffermode_flag && !am_getconfig_bool("media.libplayer.wfd")) {
        if (p_para->playctrl_info.buf_limited_time_ms <= 0) { /*wfd not need blocked write.*/
            p_para->playctrl_info.buf_limited_time_ms = 1000;
        }
    } else {
        if (p_para->vstream_info.has_video && p_para->astream_info.has_audio && (p_para->astream_num > 1) && (p_para->state.full_time > 0)) {
            p_para->playctrl_info.buf_limited_time_ms = am_getconfig_float_def("media.libplayer.limittime", 0);
            if ((p_para->vstream_info.video_height * p_para->vstream_info.video_width) > 1920 * 1088) {
                /**/
                p_para->playctrl_info.buf_limited_time_ms = p_para->playctrl_info.buf_limited_time_ms * 2;
            }
            log_print("[%s:%d]multiple audio switch, set buffer time to %d ms\n", __FUNCTION__, __LINE__, p_para->playctrl_info.buf_limited_time_ms);
        } else {
            p_para->playctrl_info.buf_limited_time_ms = 0; /*0 is not limited.*/
        }
    }
    if (p_para->buffering_enable && p_para->playctrl_info.buf_limited_time_ms > 0 &&
        p_para->buffering_exit_time_s * 1000 > (p_para->playctrl_info.buf_limited_time_ms - 100)) {
        p_para->playctrl_info.buf_limited_time_ms = p_para->buffering_exit_time_s * 1000 + 100;
        log_print("[%s] changed buf_limited_time_ms to %d,when buffering enable\n", __FUNCTION__, p_para->playctrl_info.buf_limited_time_ms);
    }
    {
        log_print("[%s] set buf_limited_time_ms to %d\n", __FUNCTION__, p_para->playctrl_info.buf_limited_time_ms);
        if (p_para->vstream_info.has_video) {
            if (p_para->vcodec != NULL) {
                codec_set_video_delay_limited_ms(p_para->vcodec, p_para->playctrl_info.buf_limited_time_ms);
            } else if (p_para->codec != NULL) {
                codec_set_video_delay_limited_ms(p_para->codec, p_para->playctrl_info.buf_limited_time_ms);
            }
        }
        if (p_para->astream_info.has_audio) {
            if (p_para->acodec != NULL) {
                codec_set_audio_delay_limited_ms(p_para->acodec, p_para->playctrl_info.buf_limited_time_ms);
            } else if (p_para->codec != NULL) {
                codec_set_audio_delay_limited_ms(p_para->codec, p_para->playctrl_info.buf_limited_time_ms);
            }
        }
    }
    if (p_para->pFormatCtx && p_para->pFormatCtx->iformat && p_para->pFormatCtx->iformat->name &&
        (((p_para->pFormatCtx->flags & AVFMT_FLAG_DRMLEVEL1) && (memcmp(p_para->pFormatCtx->iformat->name, "DRMdemux", 8) == 0)) ||
         (p_para->pFormatCtx->flags & AVFMT_FLAG_PR_TVP) ||
         (p_para->pFormatCtx->pb && p_para->pFormatCtx->pb->isprtvp))) {
        log_print("DRMdemux :: LOCAL_OEMCRYPTO_LEVEL -> L1 or PlayReady TVP\n");
        if (p_para->vcodec) {
            log_print("DRMdemux setdrmmodev vcodec\n");
            codec_set_drmmode(p_para->vcodec, 1);
        }
        if (p_para->acodec) {
            log_print("DRMdemux setdrmmodev acodec\n");
            codec_set_drmmode(p_para->acodec, 1);
        }
        if (p_para->codec) {
            log_print("DRMdemux setdrmmodev codec\n");
            codec_set_drmmode(p_para->codec, 1);
        }
    }

    return PLAYER_SUCCESS;
failed:
    return ret;
}

int player_frames_in_ff_fb(int factor)
{
    switch (factor) {
        case 2:
        case -2:
            return 1;
        case 4:
        case -4:
            return 2;
        case 8:
        case -8:
            return 4;
        case 16:
        case -16:
            return 6;
        case 32:
        case -32:
            return 8;
        case 64:
        case -64:
            return 10;
        default:
            return 1;
    }  
}
