/**
 * \file adec-external-ctrl.h
 * \brief  Function prototypes of Audio Dec
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_EXTERNAL_H
#define ADEC_EXTERNAL_H

#ifdef  __cplusplus
extern "C"
{
#endif

    int audio_decode_init(void **handle, arm_audio_info *pcodec);
    int audio_decode_start(void *handle);
    int audio_decode_pause(void *handle);
    int audio_decode_resume(void *handle);
    int audio_decode_stop(void *handle);
    int audio_decode_release(void **handle);
    int audio_decode_automute(void *, int);
    int audio_decode_set_mute(void *handle, int);
    int audio_decode_set_volume(void *, float);
    int audio_decode_get_volume(void *, float *);
    int audio_channels_swap(void *);
    int audio_channel_left_mono(void *);
    int audio_channel_right_mono(void *);
    int audio_channel_stereo(void *);
    int audio_output_muted(void *handle);
    int audio_dec_ready(void *handle);
    int audio_get_decoded_nb_frames(void *handle);
    int audio_get_error_nb_frames(void *handle);

    int audio_decode_set_lrvolume(void *, float lvol, float rvol);
    int audio_decode_get_lrvolume(void *, float* lvol, float* rvol);
    int audio_set_av_sync_threshold(void *, int);
    int audio_get_soundtrack(void *, int*);
    int get_audio_decoder(void);
    int get_decoder_status(void *p, struct adec_status *adec);
    int get_decoder_info(void *p);
    int audio_channel_lrmix_flag_set(void *, int enable);
    int audio_decpara_get(void *handle, int *pfs, int *pch);
    typedef int (*adec_player_notify_t)(int pid, int msg, unsigned long ext1, unsigned long ext2);
    int audio_register_notify(const adec_player_notify_t notify_fn);
    int audio_notify(void *handle, int msg, unsigned long ext1, unsigned long ext2);

#ifdef  __cplusplus
}
#endif

#endif
