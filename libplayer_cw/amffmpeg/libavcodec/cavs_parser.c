/*
 * Chinese AVS video (AVS1-P2, JiZhun profile) parser.
 * Copyright (c) 2006  Stefan Gehrer <stefan.gehrer@gmx.de>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Chinese AVS video (AVS1-P2, JiZhun profile) parser
 * @author Stefan Gehrer <stefan.gehrer@gmx.de>
 */

#include "parser.h"
#include "cavs.h"


/**
 * finds the end of the current frame in the bitstream.
 * @return the position of the first byte of the next frame, or -1
 */
static int cavs_find_frame_end(ParseContext *pc, const uint8_t *buf,
                               int buf_size) {
    int pic_found, i;
    uint32_t state;

    pic_found= pc->frame_start_found;
    state= pc->state;

    i=0;
    if(!pic_found){
        for(i=0; i<buf_size; i++){
            state= (state<<8) | buf[i];
            if(state == PIC_I_START_CODE || state == PIC_PB_START_CODE){
                i++;
                pic_found=1;
                break;
            }
        }
    }

    if(pic_found){
        /* EOF considered as end of frame */
        if (buf_size == 0)
            return 0;
        for(; i<buf_size; i++){
            state= (state<<8) | buf[i];
            if((state&0xFFFFFF00) == 0x100){
                if(state > SLICE_MAX_START_CODE){
                    pc->frame_start_found=0;
                    pc->state=-1;
                    return i-3;
                }
            }
        }
    }
    pc->frame_start_found= pic_found;
    pc->state= state;
    return END_NOT_FOUND;
}

static int cavsvideo_parse(AVCodecParserContext *s,
                           AVCodecContext *avctx,
                           const uint8_t **poutbuf, int *poutbuf_size,
                           const uint8_t *buf, int buf_size)
{
    ParseContext *pc = s->priv_data;
    int next;

    if(s->flags & PARSER_FLAG_COMPLETE_FRAMES){
        next= buf_size;
    }else{
        next= cavs_find_frame_end(pc, buf, buf_size);

        if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
            *poutbuf = NULL;
            *poutbuf_size = 0;
            return buf_size;
        }
    }

    if(*buf == 0x0 && *(buf+1) == 0x0 && *(buf+2) == 0x1 && (*(buf+3) == 0xb0 || *(buf+3) == 0xb3)){
        s->pict_type         = AV_PICTURE_TYPE_I;
        s->key_frame         = 1;
        av_log(NULL, AV_LOG_ERROR, "[%s %d] avs I frame\n", __FUNCTION__, __LINE__);
    }else if(*buf == 0x0 && *(buf+1) == 0x0 && *(buf+2) == 0x1 && *(buf+3) == 0xb6){
        s->pict_type         = AV_PICTURE_TYPE_P;
        s->key_frame         = -1;
    }else{
        s->pict_type         = AV_PICTURE_TYPE_P;
        s->key_frame         = -1;
    }

    *poutbuf = buf;
    *poutbuf_size = buf_size;
    return next;
}

static int cavsvideo_parse_init(AVCodecParserContext *s)
{
    av_log(NULL, AV_LOG_ERROR, "[%s %d]\n", __FUNCTION__, __LINE__);
    return 0;
}


AVCodecParser ff_cavsvideo_parser = {
    { CODEC_ID_CAVS },
    sizeof(ParseContext1),
    cavsvideo_parse_init,
    cavsvideo_parse,
    ff_parse1_close,
    ff_mpeg4video_split,
};
