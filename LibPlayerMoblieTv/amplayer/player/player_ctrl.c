/**
 * @file        player_ctrl.c
 * @brief
 * @author      Xu Hui <hui.xu@amlogic.com>
 * @version     1.0.1
 * @date        2012-01-19
 */

/* Copyright (c) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */

#include <pthread.h>
#include <player.h>
#include <player_set_sys.h>

#include "player_ts.h"
#include "player_es.h"
#include "player_rm.h"
#include "player_ps.h"
#include "player_video.h"
#include "player_audio.h"

#include "player_update.h"
#include "thread_mgt.h"
#include "player_ffmpeg_ctrl.h"
#include "player_cache_mgt.h"
#include "player_priv.h"
#include <amthreadpool.h>
#include "udrm.h"
#include "message.h"

#ifndef FBIOPUT_OSD_SRCCOLORKEY
#define  FBIOPUT_OSD_SRCCOLORKEY    0x46fb
#endif

#ifndef FBIOPUT_OSD_SRCKEY_ENABLE
#define  FBIOPUT_OSD_SRCKEY_ENABLE  0x46fa
#endif

extern void print_version_info();
int auto_refresh_rate_enable = 0;

static int udrm_callback(int error_num, void *cb_data)
{
    play_para_t *player_para = (play_para_t *)cb_data;
    log_print("[libplayer_udrm callback] error_num=%d\n", error_num);
    if (player_para != NULL)
        send_event(player_para, PLAYER_EVENTS_UDRM_MSG, (unsigned long)error_num, 0);

    return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @function    player_init
 *
 * @brief       Amlogic player initilization. Make sure call it once when
 *              setup amlogic player every time
 * @param       void
 *
 * @return      PLAYER_SUCCESS   success
 *
 * @details     register all formats and codecs;
 *              player id pool initilization;
 *              audio basic initilization;
 *              register support decoder(ts,es,rm,pure audio, pure video);
 *              keep last frame displaying for default;
 *              enable demux and set demux channel;
 */
/* --------------------------------------------------------------------------*/

int player_init(void)
{
    reset_auto_refresh_rate();
    print_version_info();
    update_loglevel_setting();
    /*register all formats and codecs*/
    ffmpeg_init();

    player_id_pool_init();

    codec_audio_basic_init();

    /*register all support decoder */
    ts_register_stream_decoder();
    es_register_stream_decoder();
    ps_register_stream_decoder();
    rm_register_stream_decoder();
    audio_register_stream_decoder();
    video_register_stream_decoder();
    udrm_init();
    return PLAYER_SUCCESS;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_start
 *
 * @brief       Amlogic player start to play a specified path streaming file.
 *
 * @param[in]   ctrl_p  player control parameters structure pointer
 * @param[in]   priv    Player unique identification
 *
 * @return      pid  current player tag
 *
 * @details     request id for current player;
 *              if not set displast_frame, or change file ,set black out;
 *              creat player thread for playback;
 */
/* --------------------------------------------------------------------------*/
int player_start(play_control_t *ctrl_p, unsigned long  priv)
{
    int ret;
    int pid = -1;
    play_para_t *p_para;
    char url[1024]={0};
#if 1
    ret = am_getconfig("libplayer.testurl", url, NULL);
    if (ret > 0) {
        strcpy(ctrl_p->file_name, url);
        ctrl_p->auto_buffing_enable =1;
    }
#endif
    //char stb_source[32];

    update_loglevel_setting();
    update_dump_dir_path();
    //print_version_info();
    log_print("[player_start:enter]p=%p black=%d\n", ctrl_p, get_black_policy());

    if (ctrl_p == NULL) {
        return PLAYER_EMPTY_P;
    }

    /*keep last frame displaying --default*/
    set_black_policy(0);
    /* if not set keep last frame, or change file playback, clear display last frame */
    if (!ctrl_p->displast_frame) {
        set_black_policy(1);
    } else if (!check_file_same(ctrl_p->file_name)) {
        set_black_policy(1);
    }
    auto_refresh_rate_enable = get_auto_refresh_rate();
    pid = player_request_pid();
    if (pid < 0) {
        return PLAYER_NOT_VALID_PID;
    }

    p_para = MALLOC(sizeof(play_para_t));
    if (p_para == NULL) {
        return PLAYER_NOMEM;
    }

    MEMSET(p_para, 0, sizeof(play_para_t));

    /* init time_point to a invalid value */
    p_para->playctrl_info.time_point = -1;

    player_init_pid_data(pid, p_para);

    message_pool_init(p_para);

    p_para->start_param = ctrl_p;
    p_para->player_id = pid;
    p_para->extern_priv = priv;
    log_debug1("[player_start]player_para=%p,start_param=%p pid=%d\n", p_para, p_para->start_param, pid);

    ret = player_thread_create(p_para) ;
    if (ret != PLAYER_SUCCESS) {
        FREE(p_para);
        player_release_pid(pid);
        return PLAYER_CAN_NOT_CREAT_THREADS;
    }
    log_print("[player_start:exit]pid = %d \n", pid);
    udrm_set_msg_func(udrm_callback, (void *)p_para);

    return pid;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_start_play
 *
 * @brief       if need_start set 1, call player_start_play to start playback
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID playet tag invalid
 *              PLAYER_NOMEM        alloc memory failed
 *              PLAYER_SUCCESS      success
 *
 * @details     if need_start set 0, no need call player_start_play
 */
/* --------------------------------------------------------------------------*/
int player_start_play(int pid)
{
    player_cmd_t *cmd;
    int r = PLAYER_SUCCESS;
    play_para_t *player_para;

    log_print("[player_start_play:enter]pid=%d\n", pid);

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }

    cmd = message_alloc();
    if (cmd) {
        cmd->ctrl_cmd = CMD_START;
        r = send_message(player_para, cmd);
    } else {
        r = PLAYER_NOMEM;
    }

    player_close_pid_data(pid);
    log_print("[player_start_play:exit]pid = %d\n", pid);

    return r;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_stop
 *
 * @brief       send stop command to player (synchronous)
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     if player already stop, return directly
 *              wait thread exit after send stop command
 */
/* --------------------------------------------------------------------------*/

#define HDMI_AVMUTE_SET_PATH  "/sys/class/amhdmitx/amhdmitx0/avmute"
#define HDMI_HDR_SRC_FEATURE_PATH  "/sys/class/amhdmitx/amhdmitx0/hdr_src_feature"

int player_stop(int pid)
{
    int fd,fd1 = -1;
    int bytes;
    char  buffer[16] = {0};
    player_cmd_t *cmd;
    int r = PLAYER_SUCCESS;
    play_para_t *player_para;
    player_status sta;

    log_print("[player_stop:enter]pid=%d\n", pid);

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }
    
    sta = get_player_state(player_para);
    log_print("[player_stop]player_status=%x\n", sta);
    if (PLAYER_THREAD_IS_STOPPED(sta)) {
        player_close_pid_data(pid);
        log_print("[player_stop]pid=%d thread is already stopped\n", pid);
        return PLAYER_SUCCESS;
    }
    /*if (player_para->pFormatCtx) {
        av_ioctrl(player_para->pFormatCtx, AVIOCTL_STOP, 0, 0);
    }*/

     //only for handle quit hdr mode in local player.
    if (player_para->pFormatCtx&&player_para->pFormatCtx->pb&&
        player_para->pFormatCtx->pb->local_playback) {
       fd = open(HDMI_HDR_SRC_FEATURE_PATH,O_RDONLY,0644);
       if (fd >= 0) {
           bytes = read(fd, buffer, sizeof(buffer));
           close(fd);
           if (bytes && 0 == strncmp("1",buffer,1)){
              fd1 = open(HDMI_AVMUTE_SET_PATH,O_RDWR, 0644);
              if (fd1 >= 0){
                  sprintf(buffer,"%d",1);
                  log_print("set avmute");
                  write(fd1,buffer,strlen(buffer));
              }
           }
       }else {
           log_print("unable to open file %s \n", HDMI_HDR_SRC_FEATURE_PATH);
       }
    }
    
    clear_all_message(player_para);/*clear old message to make sure fast exit.*/
    cmd = message_alloc();
    if (cmd) {
        cmd->ctrl_cmd = CMD_STOP;
        ffmpeg_interrupt(player_para->thread_mgt.pthread_id);
        r = send_message(player_para, cmd);
        r = player_thread_wait_exit(player_para);
        log_print("[player_stop:%d]wait player_theadpid[%d] r = %d\n", __LINE__, player_para->player_id, r);
        clear_all_message(player_para);
        ffmpeg_uninterrupt(player_para->thread_mgt.pthread_id);
    } else {
        r = PLAYER_NOMEM;
    }

    player_close_pid_data(pid);
    log_print("[player_stop:exit]pid=%d\n", pid);
    tcppool_refresh_link_and_check();
    log_print("[tcppool_refresh_link_and_check]pid=%d\n", pid);

    if (fd1 >= 0){
       sprintf(buffer,"%d",-1);
       usleep(1000 * 200);
       log_print("clear avmute");
       write(fd1,buffer,strlen(buffer));
       close(fd1);
    }
    return r;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_stop_async
 *
 * @brief       send stop command to player (asynchronous)
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     if player already stop, return directly
 *              needn't wait thread exit
 */
/* --------------------------------------------------------------------------*/
int player_stop_async(int pid)
{
    player_cmd_t *cmd;
    int r = PLAYER_SUCCESS;
    play_para_t *player_para;
    player_status sta;

    player_para = player_open_pid_data(pid);

    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }

    sta = get_player_state(player_para);
    log_print("[player_stop]player_status=%x\n", sta);
    if (PLAYER_THREAD_IS_STOPPED(sta)) {
        player_close_pid_data(pid);
        log_print("[player_stop]pid=%d thread is already stopped\n", pid);
        return PLAYER_SUCCESS;
    }
    clear_all_message(player_para);/*clear old message to make sure fast exit.*/
    cmd = message_alloc();
    if (cmd) {
        cmd->ctrl_cmd = CMD_STOP;
        ffmpeg_interrupt(player_para->thread_mgt.pthread_id);
        r = send_message(player_para, cmd);
    } else {
        r = PLAYER_NOMEM;
    }

    player_close_pid_data(pid);

    return r;
}




/* --------------------------------------------------------------------------*/
/**
 * @function    player_exit
 *
 * @brief       release player resource
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     player_exit must with pairs of player_play
 */
/* --------------------------------------------------------------------------*/
int player_exit(int pid)
{
    int ret = PLAYER_SUCCESS;
    play_para_t *para;

    log_print("[player_exit:enter]pid=%d\n", pid);

    para = player_open_pid_data(pid);
    if (para != NULL) {
        log_print("[player_exit]player_state=0x%x\n", get_player_state(para));
        if (get_player_state(para) != PLAYER_EXIT) {
            player_stop(pid);
        }

        ret = player_thread_wait_exit(para);
        log_print("[player_exit]player thread already exit: %d\n", ret);
        ffmpeg_uninterrupt(para->thread_mgt.pthread_id);
        FREE(para);
        para = NULL;
    }
    player_close_pid_data(pid);
    player_release_pid(pid);
    udrm_set_msg_func(NULL, NULL);
    log_print("[player_exit:exit]pid=%d\n", pid);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_pause
 *
 * @brief       send pause command to player
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     null
 */
/* --------------------------------------------------------------------------*/
int player_pause(int pid)
{
    player_cmd_t cmd;
    int ret = PLAYER_SUCCESS;

    log_print("[player_pause:enter]pid=%d\n", pid);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_PAUSE;

    ret = player_send_message(pid, &cmd);
    log_print("[player_pause:exit]pid=%d ret=%d\n", pid, ret);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_resume
 *
 * @brief       send resume command to player
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     null
 */
/* --------------------------------------------------------------------------*/
int player_resume(int pid)
{
    player_cmd_t cmd;
    int ret;

    log_print("[player_resume:enter]pid=%d\n", pid);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_RESUME;

    ret = player_send_message(pid, &cmd);
    log_print("[player_resume:exit]pid=%d ret=%d\n", pid, ret);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_loop
 *
 * @brief       send loop command to set loop play current file
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     need set loop before stream play end
 */
/* --------------------------------------------------------------------------*/
int player_loop(int pid)
{
    player_cmd_t cmd;
    int ret;

    log_print("[player_loop:enter]pid=%d\n", pid);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.set_mode = CMD_LOOP;

    ret = player_send_message(pid, &cmd);
    log_print("[player_loop:exit]pid=%d ret=%d\n", pid, ret);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_noloop
 *
 * @brief       send noloop command to cancle loop play
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details need cancel loop before stream play end
 */
/* --------------------------------------------------------------------------*/

int player_noloop(int pid)
{
    player_cmd_t cmd;
    int ret;

    log_print("[player_loop:enter]pid=%d\n", pid);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.set_mode = CMD_NOLOOP;

    ret = player_send_message(pid, &cmd);
    log_print("[player_loop:exit]pid=%d ret=%d\n", pid, ret);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_timesearch
 *
 * @brief       seek to designated time point to play.
 *
 * @param[in]   pid player tag which get from player_start return value
 * @param[in]   s_time target time, unit is second
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     After time search, player playback from a key frame
 */
/* --------------------------------------------------------------------------*/
int player_timesearch(int pid, float s_time)
{
    player_cmd_t cmd;
    int ret;
    log_print("[player_timesearch:enter]pid=%d s_time=%f\n", pid, s_time);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_SEARCH;
    cmd.f_param = s_time;

    ret = player_send_message(pid, &cmd);
    log_print("[player_timesearch:exit]pid=%d ret=%d\n", pid, ret);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_forward
 *
 * @brief       send fastforward command to player
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   speed   fast forward step
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     After ff, player playback from a key frame
 */
/* --------------------------------------------------------------------------*/
int player_forward(int pid, int speed)
{
    player_cmd_t cmd;
    int ret;

    log_print("[player_forward:enter]pid=%d speed=%d\n", pid, speed);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_FF;
    cmd.param = speed;

    ret = player_send_message(pid, &cmd);
    log_print("[player_forward:exit]pid=%d ret=%d\n", pid, ret);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_backward
 *
 * @brief       send fast backward command to player.
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   speed   fast backward step
 *
 * @return      PLAYER_NOT_VALID_PID playet tag invalid
 *              PLAYER_NOMEM        alloc memory failed
 *              PLAYER_SUCCESS      success
 *
 * @details     After fb, player playback from a key frame
 */
/* --------------------------------------------------------------------------*/
int player_backward(int pid, int speed)
{
    player_cmd_t cmd;
    int ret;

    log_print("[player_backward:enter]pid=%d speed=%d\n", pid, speed);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_FB;
    cmd.param = speed;

    ret = player_send_message(pid, &cmd);
    log_print("[player_backward]cmd=%x param=%d ret=%d\n", cmd.ctrl_cmd, cmd.param, ret);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_aid
 *
 * @brief       switch audio stream to designed id audio stream.
 *
 * @param[in]   pid         player tag which get from player_start return value
 * @param[in]   audio_id    target audio stream id,
 *                          can find through media_info command
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     audio_id is audio stream index
 */
/* --------------------------------------------------------------------------*/
int player_aid(int pid, int audio_id, int audio_idx)
{
    player_cmd_t cmd;

    int ret;

    log_print("[player_aid:enter]pid=%d aid=%d, audio_idx=%d\n", pid, audio_id, audio_idx);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_SWITCH_AID;
    cmd.param = audio_id;
    cmd.param1 = audio_idx;
    if (player_get_state(pid) >= PLAYER_INITOK && player_get_state(pid) < PLAYER_RUNNING) {
        play_para_t *player_para;
        player_para = player_open_pid_data(pid);
        if (player_para != NULL && player_pre_switch_audio(player_para, audio_id) == 0) {
            return 0;//switched ok.
        }
    }
    ret = player_send_message(pid, &cmd);
    log_print("[player_aid:exit]pid=%d ret=%d\n", pid, ret);

    return ret;

}

int player_switch_program(int pid, int video_pid, int audio_pid)
{
    player_cmd_t cmd;

    int ret;

    log_print("[player_switch_program:enter]pid=%d video_pid=%d audio_pid=%d CMD_SWITCH_TSPROGRAM:%x\n",
        pid, video_pid, audio_pid, CMD_SWITCH_TSPROGRAM);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_SWITCH_TSPROGRAM;
    cmd.param = video_pid;
    cmd.param1= audio_pid;

    ret = player_send_message(pid, &cmd);
    log_print("[player_switch_program:exit]pid=%d ret=%d\n", pid, ret);

    return ret;

}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_sid
 *
 * @brief       send switch subtitle id command to player
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   sub_id  target subtitle stream id,
 *                      can find through media_info command
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     sub_id is subtitle stream index
 */
/* --------------------------------------------------------------------------*/
int player_sid(int pid, int sub_id)
{
    player_cmd_t cmd;
    int ret;

    log_print("[player_sid:enter]pid=%d sub_id=%d\n", pid, sub_id);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.ctrl_cmd = CMD_SWITCH_SID;
    cmd.param = sub_id;

    ret = player_send_message(pid, &cmd);
    log_print("[player_sid:exit]pid=%d sub_id=%d\n", pid, sub_id);

    return ret;

}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_enable_autobuffer
 *
 * @brief       enable/disable auto buffering
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   enable  enable/disable auto buffer function
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     if enable auto buffering, need set limit use player_set_autobuffer_level.
 */
/* --------------------------------------------------------------------------*/
int player_enable_autobuffer(int pid, int enable)
{
    player_cmd_t cmd;
    int ret;

    log_print("[%s:enter]pid=%d enable=%d\n", __FUNCTION__, pid, enable);

    MEMSET(&cmd, 0, sizeof(player_cmd_t));

    cmd.set_mode = CMD_EN_AUTOBUF;
    cmd.param = enable;

    ret = player_send_message(pid, &cmd);
    log_print("[%s:exit]pid=%d enable=%d\n", __FUNCTION__, pid, enable);

    return ret;

}

/* --------------------------------------------------------------------------*/
/**
 * @function   player_set_autobuffer_level
 *
 * @brief   player_set_autobuffer_level
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   min     buffer min percent (less than min, enter buffering, av pause)
 * @param[in]   middle  buffer middle percent(more than middler, exit buffering, av resume)
 * @param[in]   max     buffer max percent(more than max, do not feed data)
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     if buffer level low than min, player auto pause to buffer data,
 *              if buffer level high than middle, player auto reusme playback
 */
/* --------------------------------------------------------------------------*/
int player_set_autobuffer_level(int pid, float min, float middle, float max)
{
    player_cmd_t cmd;
    int ret;

    log_print("[%s:enter]pid=%d min=%.3f middle=%.3f max=%.3f\n", __FUNCTION__, pid, min, middle, max);

    if (min <  middle && middle < max && max < 1) {
        MEMSET(&cmd, 0, sizeof(player_cmd_t));

        cmd.set_mode = CMD_SET_AUTOBUF_LEV;
        cmd.f_param = min;
        cmd.f_param1 = middle;
        cmd.f_param2 = max;

        ret = player_send_message(pid, &cmd);
    } else {
        ret = -1;
        log_error("[%s]invalid param, please check!\n", __FUNCTION__);
    }
    log_print("[%s:exit]pid=%d min=%.3f middle=%.3f max=%.3f\n", __FUNCTION__, pid, min, middle, max);

    return ret;

}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_send_message
 *
 * @brief       send message to player thread
 *
 * @param[in]   pid player tag which get from player_start return value
 * @param[in]   cmd player control command
 *
 * @return      PLAYER_NOT_VALID_PID    playet tag invalid
 *              PLAYER_NOMEM            alloc memory failed
 *              PLAYER_SUCCESS          success
 *
 * @details     if player has exited, send message invalid
 */
/* --------------------------------------------------------------------------*/
int player_send_message(int pid, player_cmd_t *cmd)
{
    player_cmd_t *mycmd;
    int r = -1;
    play_para_t *player_para;
    char buf[512];

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }

    if (player_get_state(pid) == PLAYER_EXIT) {
        player_close_pid_data(pid);
        return PLAYER_SUCCESS;
    }

    mycmd = message_alloc();
    if (mycmd) {
        memcpy(mycmd, cmd, sizeof(*cmd));
        r = send_message_by_pid(pid, mycmd);
        if (cmd2str(cmd, buf) != -1) {
            log_print("[%s]cmd = %s\n", __FUNCTION__, buf);
        }
    } else {
        r = PLAYER_NOMEM;
    }
    player_close_pid_data(pid);
    return r;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_register_update_callback
 *
 * @brief       App can register a update callback function into player
 *
 * @param[in]   cb          callback structure point
 * @param[in]   up_fn       update function
 * @param[in]   interval_s  update interval (milliseconds)
 *
 * @return      PLAYER_EMPTY_P          invalid pointer
 *              PLAYER_ERROR_CALLBACK   up_fn invalid
 *              PLAYER_SUCCESS          success
 *
 * @details     used to update player status
 */
/* --------------------------------------------------------------------------*/
int player_register_update_callback(callback_t *cb, update_state_fun_t up_fn, int interval_s)
{
    int ret;
    if (!cb) {
        log_error("[player_register_update_callback]empty callback pointer!\n");
        return PLAYER_EMPTY_P;
    }

    ret = register_update_callback(cb, up_fn, interval_s);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_state
 *
 * @brief       get player current state
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      status  player current status
 *              PLAYER_NOT_VALID_PID error,invalid pid
 *
 * @details     state defined in player_type.h
 */
/* --------------------------------------------------------------------------*/
player_status player_get_state(int pid)
{
    player_status status;
    play_para_t *player_para;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }

    status = get_player_state(player_para);
    player_close_pid_data(pid);

    return status;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_extern_priv
 *
 * @brief       get current player's unique identification
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      externed                player's unique identification
 *              PLAYER_NOT_VALID_PID    error,invalid pid
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
unsigned int player_get_extern_priv(int pid)
{
    unsigned long externed;
    play_para_t *player_para;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;    /*this data is 0 for default!*/
    }

    externed = player_para->extern_priv;
    player_close_pid_data(pid);

    return externed;
}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_play_info
 *
 * @brief       get player's information
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[out]  info    play info structure pointer
 *
 * @return      PLAYER_SUCCESS          success
 *              PLAYER_NOT_VALID_PID    error,invalid pid
 *
 * @details     get playing information,status, current_time, buferlevel etc.
 */
/* --------------------------------------------------------------------------*/
int player_get_play_info(int pid, player_info_t *info)
{
    play_para_t *player_para;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;    /*this data is 0 for default!*/
    }

    MEMSET(info, 0, sizeof(player_info_t));
    MEMCPY(info, &player_para->state, sizeof(player_info_t));
    player_close_pid_data(pid);

    return PLAYER_SUCCESS;
}
/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_lpbufbuffedsize
 *
 * @brief       get player current lpbufbuffedsize
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      plbuffedsize;
 *
 * @details     state defined in player_type.h
 */
/* --------------------------------------------------------------------------*/
int64_t player_get_lpbufbuffedsize(int pid)
{
    int64_t buffedsize = -1;
    play_para_t *player_para;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }

    buffedsize = getlpbuffer_buffedsize(player_para);
    player_close_pid_data(pid);

    return buffedsize;
}
/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_streambufbuffedsize
 *
 * @brief       get player current streambufbuffedsize
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      streambufbuffedsize;
 *
 * @details     state defined in player_type.h
 */
/* --------------------------------------------------------------------------*/
int64_t player_get_streambufbuffedsize(int pid)
{
    int64_t buffedsize = -1;
    play_para_t *player_para;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;
    }

    buffedsize = getstreambuffer_buffedsize(player_para);
    player_close_pid_data(pid);

    return buffedsize;
}

/* --------------------------------------------------------------------------*/
/**
 * @fucntion    player_get_media_info
 *
 * @brief       get file media information
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[out]  minfo   media info structure pointer
 *
 * @return      PLAYER_SUCCESS          success
 *              PLAYER_NOT_VALID_PID    error,invalid pid
 *
 * @details     get file media information, such as audio format, video format, etc.
 */
/* --------------------------------------------------------------------------*/
int player_get_media_info(int pid, media_info_t *minfo)
{
    play_para_t *player_para;
    player_status sta;

    while (player_get_state(pid) < PLAYER_INITOK) {
        sta = player_get_state(pid);
        if (sta == NULL) {
            log_error("player_get_media_info failed pid [%d]\n", pid);
            return PLAYER_FAILED;
        }
        if (sta >= PLAYER_ERROR && sta <= PLAYER_EXIT) {
            player_close_pid_data(pid);
            log_error("player_get_media_info status err [0x%x]\n", sta);
            return PLAYER_INVALID_CMD;
        }
        if ((player_get_state(pid)) == PLAYER_ERROR ||
            player_get_state(pid) == PLAYER_STOPED ||
            player_get_state(pid) == PLAYER_PLAYEND ||
            player_get_state(pid) == PLAYER_EXIT) {
            log_error("player_get_media_info failed status [0x%x]\n", sta);
            return PLAYER_FAILED;
        }
        usleep(1000 * 10);
    }

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;    /*this data is 0 for default!*/
    }

    MEMSET(minfo, 0, sizeof(media_info_t));
    MEMCPY(minfo, &player_para->media_info, sizeof(media_info_t));

    log_print("[player_get_media_info]video_num=%d vidx=%d\n", minfo->stream_info.total_video_num, minfo->stream_info.cur_video_index);
    player_close_pid_data(pid);

    return PLAYER_SUCCESS;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_video_overlay_en
 *
 * @brief       enable osd colorkey
 *
 * @param[in]   enable  osd colorkey enable flag
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
int player_video_overlay_en(unsigned enable)
{
    int fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd >= 0) {
        unsigned myKeyColor = 0;
        unsigned myKeyColor_en = enable;

        if (myKeyColor_en) {
            myKeyColor = 0xff;/*set another value to solved the bug in kernel..remove later*/
            ioctl(fd, FBIOPUT_OSD_SRCCOLORKEY, &myKeyColor);
            myKeyColor = 0;
            ioctl(fd, FBIOPUT_OSD_SRCCOLORKEY, &myKeyColor);
            ioctl(fd, FBIOPUT_OSD_SRCKEY_ENABLE, &myKeyColor_en);
        } else {
            ioctl(fd, FBIOPUT_OSD_SRCKEY_ENABLE, &myKeyColor_en);
        }
        close(fd);
        return PLAYER_SUCCESS;
    }
    return PLAYER_FAILED;
}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_set_disp_lastframe
 *
 * @brief       set  display lastframe
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   enable  display lastframe
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
int player_set_disp_lastframe(int pid, int disp)
{
    log_print("[player_set_disp_lastframe]disp=%d", disp);

    play_para_t *player_para;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;    /*this data is 0 for default!*/
    }
    player_para->playctrl_info.black_out = !disp;
    //set_black_policy(player_para->playctrl_info.black_out);
    player_close_pid_data(pid);
    return PLAYER_SUCCESS;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_bitrate
 *
 * @brief       get video+audio bitrate
 *
 * @param[in]   pid     player tag which get from player_start return value
 *
 * @return      bitrate video bitrate + audio bitrate
 *              PLAYER_NOT_VALID_PID    error,invalid pid
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
int player_get_bitrate(int pid)
{
    int vrate = 0, arate = 0;
    play_para_t *player_para;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return PLAYER_NOT_VALID_PID;    /*this data is 0 for default!*/
    }
#if 1
    ffmpeg_geturl_netstream_info(player_para, 2, &vrate);
    if (vrate == 0) {
        if (player_para->state.full_time > 0)
            vrate = (player_para->file_size*8)/player_para->state.full_time;
    }
#else
    if (player_para->codec) {
        codec_get_audio_checkin_bitrate(player_para->codec, &arate);
        codec_get_video_checkin_bitrate(player_para->codec, &vrate);
    } else {
        if (player_para->vcodec) {
            codec_get_video_checkin_bitrate(player_para->vcodec, &vrate);
        }
        if (player_para->acodec) {
            codec_get_audio_checkin_bitrate(player_para->acodec, &arate);
        }
    }
#endif
    player_close_pid_data(pid);
    //log_print("[player_get_bitrate] vrate %d, arate %d, total rate %d\n", vrate, arate, vrate+arate);
    return vrate + arate;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_set_mute
 *
 * @brief       volume mute switch
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   mute_on volume mute flag 1:mute 0:inmute
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details
 */
/* --------------------------------------------------------------------------*/

int audio_set_mute(int pid, int mute_on)
{

    int ret = PLAYER_FAILED;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para != NULL) {
        player_para->playctrl_info.audio_mute = mute_on & 0x1;
        log_print("[audio_set_mute:%d]muteon=%d audio_mute=%d\n", __LINE__, mute_on, player_para->playctrl_info.audio_mute);

        p = get_audio_codec(player_para);
        if (p != NULL) {
            ret = codec_set_mute(p, mute_on);
        }
        player_close_pid_data(pid);
    } else {
        ret = codec_set_mute(NULL, mute_on);
    }

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_get_volume_range
 *
 * @brief       get volume range
 *
 * @param[in]   pid player tag which get from player_start return value
 * @param[out]  min volume minimum
 * @param[out]  max volume maximum
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details     0~1
 */
/* --------------------------------------------------------------------------*/
int audio_get_volume_range(int pid, float *min, float *max)
{
    return codec_get_volume_range(NULL, min, max);
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_set_volume
 *
 * @brief       set val to volume
 *
 * @param[in]   pid player tag which get from player_start return value
 * @param[in]   val volume value
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details     val range: 0~1
 */
/* --------------------------------------------------------------------------*/
int audio_set_volume(int pid, float val)
{
    return codec_set_volume(NULL, val);
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_get_volume
 *
 * @brief       get volume
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      r = 0  success
 *
 * @details     vol range:0~1
 */
/* --------------------------------------------------------------------------*/
int audio_get_volume(int pid, float *vol)
{
    int r;

    r = codec_get_volume(NULL, vol);
    log_print("[audio_get_volume:%d]r=%d\n", __LINE__, r);

    return r;//codec_get_volume(NULL);
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_set_lrvolume
 *
 * @brief       set left and right volume
 *
 * @param[in]   pid player tag which get from player_start return value
 * @param[in]   lval: left volume value
 * @param[in]   rval: right volume value
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details     lvol,rvol range: 0~1
 */
/* --------------------------------------------------------------------------*/
int audio_set_lrvolume(int pid, float lvol, float rvol)
{
    play_para_t *player_para;
    log_print("[audio_set_lrvolume:enter]pid=%d\n", pid);
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_print("player ID is NULL!\n");
        return -1;
    }
    if (player_para->acodec == NULL) {
        log_print("codec is not ready!\n");
        return -1;
    }
    return codec_set_lrvolume(player_para->acodec, lvol, rvol);
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_get_lrvolume
 *
 * @brief       get left/right volume
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      r = 0 for success
 *
 * @details     lvol,rvol range : 0~1
 */
/* --------------------------------------------------------------------------*/
int audio_get_lrvolume(int pid, float *lvol, float* rvol)
{
    int r;

    r = codec_get_lrvolume(NULL, lvol, rvol);
    log_print("[audio_get_volume:%d]r=%d\n", __LINE__, r);

    return r;//codec_get_volume(NULL);
}



/* --------------------------------------------------------------------------*/
/**
 * @function    audio_set_volume_balance
 *
 * @brief       switch balance
 *
 * @param[in]   pid     player tag which get from player_start return value
 * @param[in]   balance balance flag    1:set balance 0:cancel balance
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
int audio_set_volume_balance(int pid, int balance)
{
    return codec_set_volume_balance(NULL, balance);
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_swap_left_right
 *
 * @brief       swap left and right channel
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
int audio_swap_left_right(int pid)
{
    return codec_swap_left_right(NULL);
}

/* --------------------------------------------------------------------------*/
/**
 * @function   audio_left_mono
 *
 * @brief
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return  PLAYER_SUCCESS  success
 *          PLAYER_FAILED   failed
 * @details
 */
/* --------------------------------------------------------------------------*/

int audio_left_mono(int pid)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return 0;    /*this data is 0 for default!*/
    }

    p = get_audio_codec(player_para);
    if (p != NULL) {
        ret = codec_left_mono(p);
    }
    player_close_pid_data(pid);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_right_mono
 *
 * @brief       audio_right_mono
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 * @details
 */
/* --------------------------------------------------------------------------*/
int audio_right_mono(int pid)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return 0;    /*this data is 0 for default!*/
    }

    p = get_audio_codec(player_para);
    if (p != NULL) {
        ret = codec_right_mono(p);
    }
    player_close_pid_data(pid);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    audio_stereo
 *
 * @brief
 *
 * @param[in]   pid player tag which get from player_start return value
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 * @details
 */
/* --------------------------------------------------------------------------*/
int audio_stereo(int pid)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return 0;    /*this data is 0 for default!*/
    }

    p = get_audio_codec(player_para);
    if (p != NULL) {
        ret = codec_stereo(p);
    }
    player_close_pid_data(pid);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
+ * @function    audio_lr_mix
+ *
+ * @brief
+ *
+ * @param[in]   pid player tag which get from player_start return value
+ *
+ * @return      PLAYER_SUCCESS  success
+ *              PLAYER_FAILED   failed+ * @details+
*/
/* --------------------------------------------------------------------------*/
int audio_lr_mix_set(int pid, int lr_mode)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_print("[%s %d] player_para==NULL,set fail audio_lr_mix!!", __FUNCTION__, __LINE__);
        return -1;    /*this data is 0 for default!*/
    }
    p = get_audio_codec(player_para);
    if (p != NULL) {
        ret = codec_lr_mix_set(p, lr_mode);
    } else {
        log_print("[%s %d] p==NULL,set fail audio_lr_mix!!", __FUNCTION__, __LINE__);
    }
    player_close_pid_data(pid);
    return ret;
}

int audio_cur_pcmpara_Applied_get(int pid, int *pfs, int *pch)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        log_print("[%s %d] player_para==NULL,set fail audio_FsNch_get!!", __FUNCTION__, __LINE__);
        return -1;    /*this data is 0 for default!*/
    }
    p = get_audio_codec(player_para);
    if (p != NULL) {
        ret = codec_pcmpara_Applied_get(p, pfs, pch);
    } else {
        log_print("[%s %d] p==NULL,set fail audio_FsNch_get!!", __FUNCTION__, __LINE__);
    }
    player_close_pid_data(pid);
    return ret;
}


/* --------------------------------------------------------------------------*/
/**
 * @function    audio_set_spectrum_switch
 *
 * @brief
 *
 * @param[in]   pid         player tag which get from player_start return value
 * @param[in]   isStart     open/close spectrum switch function
 * @param[in]   interval    swtich interval
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 * @details
 */
/* --------------------------------------------------------------------------*/
int audio_set_spectrum_switch(int pid, int isStart, int interval)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return 0;    /*this data is 0 for default!*/
    }

    p = get_audio_codec(player_para);
    if (p != NULL) {
        ret = codec_audio_spectrum_switch(p, isStart, interval);
    }
    player_close_pid_data(pid);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_progress_exit
 *
 * @brief       used for all exit,please only call at this process fatal error.
 *
 * @return      PLAYER_SUCCESS  success
 *
 * @details     Do not wait any things in this function
 */
/* --------------------------------------------------------------------------*/
int player_progress_exit(void)
{
    codec_close_audio(NULL);

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_list_allpid
 *
 * @brief       list all alived player pid
 *
 * @param[out]  pid pid list structure pointer
 *
 * @return      PLAYER_SUCCESS  success
 *              PLAYER_FAILED   failed
 *
 * @details     support multiple player threads, but only one threads use hardware decoder
 */
/* --------------------------------------------------------------------------*/

int player_list_allpid(pid_info_t *pid)
{
    char buf[MAX_PLAYER_THREADS];
    int pnum = 0;
    int i;

    pnum = player_list_pid(buf, MAX_PLAYER_THREADS);
    pid->num = pnum;

    for (i = 0; i < pnum; i++) {
        pid->pid[i] = buf[i];
    }

    return 0;
}
int player_set_inner_exit(int pid)
{
    player_set_inner_exit_pid(pid);
    return 0;
}
int player_is_inner_exit(int pid)
{
    int ret = 0;
    ret = player_is_inner_exit_pid(pid);
    return ret;
}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_cache_system_init
 *
 * @brief       player_cache_system_init
 *
 * @param[in]   enable
 * @param[in]   dir
 * @param[in]   max_size
 * @param[in]   block_size
 *
 * @return      0;
 *
 * @details
 */
/* --------------------------------------------------------------------------*/


int player_cache_system_init(int enable, const char*dir, int max_size, int block_size)
{
    return cache_system_init(enable, dir, max_size, block_size);
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_status2str
 *
 * @brief       convert player state value to string
 *
 * @param[in]   status  player status
 *
 * @return      player status details strings
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
char *player_status2str(player_status status)
{
    switch (status) {
    case PLAYER_INITING:
        return "BEGIN_INIT";

    case PLAYER_TYPE_REDY:
        return "TYPE_READY";

    case PLAYER_INITOK:
        return "INIT_OK";

    case PLAYER_RUNNING:
        return "PLAYING";

    case PLAYER_BUFFERING:
        return "BUFFERING";

    case PLAYER_BUFFER_OK:
        return "BUFFEROK";

    case PLAYER_PAUSE:
        return "PAUSE";

    case PLAYER_SEARCHING:
        return "SEARCH_FFFB";

    case PLAYER_SEARCHOK:
        return "SEARCH_OK";

    case PLAYER_START:
        return "START_PLAY";

    case PLAYER_FF_END:
        return "FF_END";

    case PLAYER_FB_END:
        return "FB_END";

    case PLAYER_ERROR:
        return "ERROR";

    case PLAYER_PLAYEND:
        return "PLAY_END";

    case PLAYER_STOPED:
        return "STOPED";

    case PLAYER_EXIT:
        return "EXIT";

    case PLAYER_PLAY_NEXT:
        return "PLAY_NEXT";

    case PLAYER_FOUND_SUB:
        return "NEW_SUB";

    case PLAYER_DIVX_AUTHORERR:
        return "DIVX_AUTHORERR";
    case PLAYER_DIVX_RENTAL_VIEW:
        return "DIVX_RENTAL";
    case PLAYER_DIVX_RENTAL_EXPIRED:
        return "DIVX_EXPIRED";
    default:
        return "UNKNOW_STATE";
    }
}

static char* player_vformat2str(vformat_t value)
{
    switch (value) {
    case VFORMAT_MPEG12:
        return "VFORMAT_MPEG12";

    case VFORMAT_MPEG4:
        return "VFORMAT_MPEG4";

    case VFORMAT_H264:
        return "VFORMAT_H264";

    case VFORMAT_HEVC:
        return "VFORMAT_HEVC";

    case VFORMAT_MJPEG:
        return "VFORMAT_MJPEG";

    case VFORMAT_REAL:
        return "VFORMAT_REAL";

    case VFORMAT_JPEG:
        return "VFORMAT_JPEG";

    case VFORMAT_VC1:
        return "VFORMAT_VC1";

    case VFORMAT_AVS:
        return "VFORMAT_AVS";

    case VFORMAT_SW:
        return "VFORMAT_SW";

    case VFORMAT_H264MVC:
        return "VFORMAT_H264MVC";

    case VFORMAT_H264_4K2K:
        return "VFORMAT_H264_4K2K";

    default:
        return "NOT_SUPPORT VFORMAT";
    }
    return NULL;
}

static char* player_aformat2str(aformat_t value)
{
    switch (value) {
    case AFORMAT_MPEG:
        return "AFORMAT_MPEG";

    case AFORMAT_PCM_S16LE:
        return "AFORMAT_PCM_S16LE";

    case AFORMAT_AAC:
        return "AFORMAT_AAC";

    case AFORMAT_AC3:
        return "AFORMAT_AC3";

    case AFORMAT_ALAW:
        return "AFORMAT_ALAW";

    case AFORMAT_MULAW:
        return "AFORMAT_MULAW";

    case AFORMAT_DTS:
        return "AFORMAT_DTS";

    case AFORMAT_PCM_S16BE:
        return "AFORMAT_PCM_S16BE";

    case AFORMAT_FLAC:
        return "AFORMAT_FLAC";

    case AFORMAT_COOK:
        return "AFORMAT_COOK";

    case AFORMAT_PCM_U8:
        return "AFORMAT_PCM_U8";

    case AFORMAT_ADPCM:
        return "AFORMAT_ADPCM";

    case AFORMAT_AMR:
        return "AFORMAT_AMR";

    case AFORMAT_RAAC:
        return "AFORMAT_RAAC";

    case AFORMAT_WMA:
        return "AFORMAT_WMA";

    case AFORMAT_WMAPRO:
        return "AFORMAT_WMAPRO";

    case AFORMAT_PCM_BLURAY:
        return "AFORMAT_PCM_BLURAY";

    case AFORMAT_ALAC:
        return "AFORMAT_ALAC";

    case AFORMAT_VORBIS:
        return "AFORMAT_VORBIS";

    case AFORMAT_AAC_LATM:
        return "AFORMAT_AAC_LATM";

    case AFORMAT_APE:
        return "AFORMAT_APE";

    case AFORMAT_EAC3:
        return "AFORMAT_EAC3";
    case AFORMAT_TRUEHD:
        return "AFORMAT_TRUEHD";
    case AFORMAT_WMAVOI:
        return "AFORMAT_WMAVOI";
    default:
        return "NOT_SUPPORT AFORMAT";
    }
    return NULL;
}
/* --------------------------------------------------------------------------*/
/**
 * @function    player_value2str
 *
 * @brief       convert player state value to string
 *
 * @param[in]  char *key valuetype
 *                       key:  status  player status;
*                                vformat video format;
*                                aformat aduio format
 * @param[in]  int value   which need convert to string
 *
 * @return      player status details strings
 *
 * @details
 */
/* --------------------------------------------------------------------------*/
char *player_value2str(char *key, int value)
{
    if (strcasecmp(key, "status") == 0) {
        return player_status2str((player_status)value);
    } else if (strcasecmp(key, "vformat") == 0) {
        return player_vformat2str((vformat_t)value);
    } else if (strcasecmp(key, "aformat") == 0) {
        return player_aformat2str((aformat_t)value);
    } else {
        return ("INVALID KEYWORDS");
    }
}

int audio_get_decoder_enable(int pid)
{

    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    p = get_audio_codec(player_para);
    if (p != NULL) {
        ret = codec_get_decoder_enable(p);
    }
    player_close_pid_data(pid);

    return ret;
}

int player_closeCodec(int pid)
{
    int ret = -1;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    codec_pause(player_para);
    return codec_close(player_para);
}

int resume_auto_refresh_rate()
{
    return set_auto_refresh_rate(auto_refresh_rate_enable);
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_sub_odata
 *
 * @brief       get subtitle data from subtitle decode
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   amsub_info; the paraments of subtitle, such as sub_type ,sub_data,etc
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/


int player_get_sub_odata(int pid, amsub_info_t *amsub_info)
{
    int ret = 0;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }

    p = get_subtitle_codec(player_para);

    if (p) {
        ret = codec_amsub_read_outdata(p, amsub_info);
        if (ret != 0) {
            log_print("player_get_sub_odata,get amsub data failed!\n");
            player_close_pid_data(pid);
            return -1;
        }
        //return 2;
    } else {
        log_print("player_get_sub_odata,can not get amsub_handle!\n");
        player_close_pid_data(pid);
        return -1;
    }
    player_close_pid_data(pid);

    return 0;

}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_sub_start_pts
 *
 * @brief       get player start time from video or audio stream
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   start_pts; the value of video or audio stream start time
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/


int player_get_sub_start_pts(int pid, unsigned int *start_pts)
{
    int ret = 0;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }

    if (player_para->astream_info.start_time > 0) {
        *start_pts = (unsigned int)player_para->astream_info.start_time;
    } else if (player_para->vstream_info.start_time > 0) {
        *start_pts = (unsigned int)player_para->vstream_info.start_time;
    } else {
        *start_pts = 0;
        player_close_pid_data(pid);
        return -1;
    }
    log_print("%s: start_pts=%d!\n", __FUNCTION__, *start_pts);
    player_close_pid_data(pid);

    return 0;

}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_set_sub_filename
 *
 * @brief       set filename for subtitle of idx+sub;
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   filename; the name of idx + sub
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/


int player_set_sub_filename(int pid, const char* filename)
{
    int ret = 0;
    play_para_t *player_para;
    codec_para_t *p;
    log_print("player_set_sub_filename,pid=%d,filename=%s !\n", pid, filename);

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    if (filename) {
        player_para->sub_filename = filename;
    }

    player_close_pid_data(pid);

    return 0;

}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_current_time
 *
 * @brief      get player current time
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   curr_timeMs; get the current time
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/


int player_get_current_time(int pid, unsigned int* curr_timeMs)
{
    int ret = 0;
    play_para_t *player_para;
    log_print("[%s]: pid=%d.\n", __FUNCTION__, pid);

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    if (player_para->state.current_ms > 0) {
        *curr_timeMs = player_para->state.current_ms;
    } else {
        *curr_timeMs = 0;
    }

    player_close_pid_data(pid);

    return 0;

}


/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_curr_sub_index
 *
 * @brief       get  current subtitle index
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   curr_sub_index; the index of current subtitle
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/


int player_get_curr_sub_id(int pid, int *curr_sub_id)
{
    int ret = 0;
    play_para_t *player_para;
    codec_para_t *p;

    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }

    *curr_sub_id = player_para->sstream_info.sub_pid;

    log_print("sstream_info.sub_pid=%d !\n", player_para->sstream_info.sub_pid);

    player_close_pid_data(pid);

    return 0;

}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_source_type
 *
 * @brief       judge current source is streaming protocol or not
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @return      1 : hls demuxer (maybe more)
 *
 */
/* --------------------------------------------------------------------------*/

int player_get_source_type(int pid)
{
    int type = 0;
    play_para_t * player_para;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    if (player_para->pFormatCtx && player_para->pFormatCtx->iformat) {
        if (!strncmp(player_para->pFormatCtx->iformat->name, "mhls", 4)) {
            type = 1;
        }
    }
    player_close_pid_data(pid);
    return type;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_streaming_track_count
 *
 * @brief       get number of valid track
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   trackNum; number of audio/sub
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/

int player_get_streaming_track_count(int pid, int * trackNum)
{
    int ret = -1;
    play_para_t * player_para;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    if (player_para->pFormatCtx && player_para->pFormatCtx->iformat && player_para->pFormatCtx->iformat->get_parameter) {
        ret = player_para->pFormatCtx->iformat->get_parameter(player_para->pFormatCtx, 3, -1, (void *)trackNum, NULL);
    }
    player_close_pid_data(pid);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_streaming_track_info
 *
 * @brief       get current streaming's audio/sub info
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   info_num; number of audio/sub info item
 *
 * @param[in]   info_t; audio/sub info array
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/

int player_get_streaming_track_info(int pid, int * info_num, AVStreamInfo *** info_t)
{
    int ret = -1;
    play_para_t * player_para;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    if (player_para->pFormatCtx && player_para->pFormatCtx->iformat && player_para->pFormatCtx->iformat->get_parameter) {
        ret = player_para->pFormatCtx->iformat->get_parameter(player_para->pFormatCtx, 1, -1, (void *)info_num, (void ***)info_t);
    }
    player_close_pid_data(pid);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_select_streaming_track
 *
 * @brief       switch audio/sub track
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   index; audio/sub track index
 *
 * @param[in]   select; select or unselect
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/

int player_select_streaming_track(int pid, int index, int select)
{
    int ret = -1;
    play_para_t * player_para;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    if (player_para->pFormatCtx && player_para->pFormatCtx->iformat && player_para->pFormatCtx->iformat->select_stream) {
        int type = 0;
        if (player_para->pFormatCtx->iformat->get_parameter) {
            player_para->pFormatCtx->iformat->get_parameter(player_para->pFormatCtx, 4, index, (void *)&type, NULL);
        }
        codec_para_t * audio_codec;
        if (type == 1) { // reset audio
            log_print("[%s:%d] start reset audio!", __FUNCTION__, __LINE__);
            unsigned int pts_audio = get_pts_audio(player_para);
            ffmpeg_set_format_codec_buffer_info(player_para, 5, (int64_t)(((float)(pts_audio) / PTS_FREQ) * 1000)); //ms
            audio_codec = get_audio_codec(player_para);
            audio_codec->automute_flag = 1;
            codec_audio_automute(audio_codec->adec_priv, 1);
            codec_close_audio(audio_codec);
            // assume audio track encoded same,
            // do not change audio parameters here,
            // maybe need to modify.
            if (!player_para->playctrl_info.raw_mode
                && player_para->astream_info.audio_format == AFORMAT_AAC) {
                ret = extract_adts_header_info(player_para);
                if (ret != PLAYER_SUCCESS) {
                    log_print("[%s:%d] extract adts header failed! ret=0x%x", __FUNCTION__, __LINE__, -ret);
                    player_close_pid_data(pid);
                    return -1;
                }
            }
            if (player_para->playctrl_info.read_end_flag) {
                player_para->playctrl_info.end_flag = 1;
                player_para->playctrl_info.streaming_track_switch_flag = 1;
            }
            audio_codec->audio_pid = 0xffff;
            if (codec_set_audio_pid(audio_codec)) {
                log_print("[%s:%d] set invalid audio pid failed!", __FUNCTION__, __LINE__);
                player_close_pid_data(pid);
                return -1;
            }
            audio_codec->has_audio = 1;
            audio_codec->audio_type = player_para->astream_info.audio_format;
            if (audio_codec->audio_type == AFORMAT_MPEG1 || audio_codec->audio_type == AFORMAT_MPEG2) {
                audio_codec->audio_type = AFORMAT_MPEG;
            }
            audio_codec->audio_pid = player_para->astream_info.audio_pid;
            audio_codec->audio_channels = player_para->astream_info.audio_channel;
            audio_codec->audio_samplerate = player_para->astream_info.audio_samplerate;
            audio_codec->switch_audio_flag = 1;
            audio_codec->audio_info.valid = 0;
            if (codec_audio_reinit(audio_codec)) {
                log_print("[%s:%d] audio reinit failed!", __FUNCTION__, __LINE__);
                player_close_pid_data(pid);
                return -1;
            }
            if (codec_reset_audio(audio_codec)) {
                log_print("[%s:%d] reset audio failed!", __FUNCTION__, __LINE__);
                player_close_pid_data(pid);
                return -1;
            }
        }

        ret = player_para->pFormatCtx->iformat->select_stream(player_para->pFormatCtx, index, select);
    }
    player_close_pid_data(pid);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_streaming_selected_track
 *
 * @brief       get current selected audio/sub track index
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   type; track type
 *
 * @param[in]   selected_track; selected audio/sub track index
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/

int player_get_streaming_selected_track(int pid, int type, int * selected_track)
{
    int ret = -1;
    play_para_t * player_para;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }
    if (player_para->pFormatCtx && player_para->pFormatCtx->iformat && player_para->pFormatCtx->iformat->get_parameter) {
        ret = player_para->pFormatCtx->iformat->get_parameter(player_para->pFormatCtx, 2, type, (void *)selected_track, NULL);
    }
    player_close_pid_data(pid);
    return ret;
}



/* --------------------------------------------------------------------------*/
/**
 * @function    player_get_play_mode
 *
 * @brief       get play mode from amplayer parament
 *
 * @param[in]   pid; player tag which get from player_start return value
 *
 * @param[in]   mode; play mode
 *
 * @param[in]   selected_track; selected audio/sub track index
 *
 * @return      r = 0 for success
 *
 */
/* --------------------------------------------------------------------------*/

int player_get_play_mode(int pid)
{
    int ret = -1;
    play_para_t * player_para;
    player_para = player_open_pid_data(pid);
    if (player_para == NULL) {
        return -1;    /*this data is 0 for default!*/
    }

	if ((player_para->pFormatCtx) && (player_para->pFormatCtx->pb) 
	    && (player_para->pFormatCtx->pb->local_playback == 1)) {
		ret = 0; // local playback
	} else if (player_para->start_param->is_livemode > 0) {
        ret = 1; // live playback
	} else {
		ret = 2; //VOD playback
	}
	
    player_close_pid_data(pid);
    return ret;
}

