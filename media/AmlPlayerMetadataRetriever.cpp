/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlPlayerMetadataRetriever"
#include "AmlPlayerMetadataRetriever.h"
#include <utils/Log.h>
#include <CharacterEncodingDetector.h>

namespace android
{

AmlPlayerMetadataRetriever::AmlPlayerMetadataRetriever()
    : mClient(NULL),
      mFileName(NULL),
      mAlbumArt(NULL),
      mParsedMetaData(false),
      mOpened(false)
{
    ALOGV("AmlPlayerMetadataRetriever()");

    mClient = new AmThumbnailInt();
    if (!mClient) {
        ALOGV("Thumbnail register decoder failed!\n");
    }

    mAmlogicFile.fd_valid = 0;
}

AmlPlayerMetadataRetriever::~AmlPlayerMetadataRetriever()
{
    ALOGV("~AmlPlayerMetadataRetriever()");

    delete mAlbumArt;
    mAlbumArt = NULL;

    if (mFileName) {
        free(mFileName);
        mFileName = NULL;
    }

    delete mClient;
    mClient = NULL;
    
    if (mAmlogicFile.fd_valid) {
        close(mAmlogicFile.fd);
    }
    mAmlogicFile.fd_valid = 0;
}

status_t AmlPlayerMetadataRetriever::setDataSource(
            const sp<IMediaHTTPService> &httpService,
            const char *url,
            const KeyedVector<String8, String8> *headers)
{
    ALOGV("setDataSource(%s)", url);
    mParsedMetaData = false;
    mMetaData.clear();
    delete mAlbumArt;
    mAlbumArt = NULL;

    return setdatasource(url, -1, 0, 0x7ffffffffffffffLL);
}

status_t AmlPlayerMetadataRetriever::setDataSource(const char * url, const KeyedVector<String8, String8> *headers)
{
    ALOGV("setDataSource(%s)", url);
    mParsedMetaData = false;
    mMetaData.clear();
    delete mAlbumArt;
    mAlbumArt = NULL;

    return setdatasource(url, -1, 0, 0x7ffffffffffffffLL);
}

status_t AmlPlayerMetadataRetriever::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
    mParsedMetaData = false;
    mMetaData.clear();

    return setdatasource(NULL, fd, offset, length);
}

status_t AmlPlayerMetadataRetriever::setdatasource(const char* url, int fd, int64_t offset, int64_t length)
{
    char* file;
    mAmlogicFile.fd_valid = 0;
	int err;

    if (url == NULL) {
        if (fd < 0 || offset < 0) {
            return -1;
        }

        file = (char *)malloc(128);
        if (file == NULL) {
            return NO_MEMORY;
        }

        mAmlogicFile.fd = dup(fd);
        mAmlogicFile.fd_valid = 1;
        mAmlogicFile.mOffset = offset;
        mAmlogicFile.mLength = length;
        sprintf(file, "amthumb:AmlogicPlayer=[%x:%x],AmlogicPlayer_fd=[%x:%x]",
                (unsigned int)this, (~(unsigned int)this), (unsigned int)&mAmlogicFile, (~(unsigned int)&mAmlogicFile));
    } else {
        file = (char *)malloc(strlen(url) + 1);
        if (file == NULL) {
            return NO_MEMORY;
        }

        strcpy(file, url);
    }

    mFileName = file;
    
    err = mClient->amthumbnail_decoder_open(mFileName);
    if (err != 0) {
        ALOGV("Thumbnail decode init failed!\n");
        free(mFileName);
        mFileName = NULL;
        return NO_INIT;
    }

    mOpened = true;
    
    return OK;
}

VideoFrame *AmlPlayerMetadataRetriever::getFrameAtTime(int64_t timeUs, int option)
{
    ALOGV("getFrameAtTime: %lld us option: %d", timeUs, option);

    int err;
    int32_t width, height;
    char *rgbbuf;
    int rotation;

    if (!mOpened) {
        err = mClient->amthumbnail_decoder_open(mFileName);
        if (err != 0) {
            ALOGV("Thumbnail decode init failed!\n");
            return NULL;
        }

        mOpened = true;
    }

    err = mClient->amthumbnail_extract_video_frame(timeUs, 0);
    if (err != 0) {
        ALOGV("Thumbnail decode frame failed, give a default pic!\n");
        VideoFrame *frameDef = new VideoFrame;
        frameDef->mWidth = 640;
        frameDef->mHeight = 480;
        frameDef->mDisplayWidth = 640;
        frameDef->mDisplayHeight = 480;
        frameDef->mSize = 640 * 480 * 2;
        frameDef->mRotationAngle = 0;
        frameDef->mData = new uint8_t[frameDef->mSize];
        memset(frameDef->mData, 0, frameDef->mSize);
        memset(frameDef->mData, 0, frameDef->mSize);
        memset(frameDef->mData, 0, frameDef->mSize);
        return frameDef;
    }

    mClient->amthumbnail_get_video_size(&width, &height);
    ALOGV("width: %d, height: %d \n", width, height);
    mClient->amthumbnail_get_video_rotation(&rotation);
    ALOGV("rotation: %d \n", rotation);


    VideoFrame *frame = new VideoFrame;
    frame->mWidth = width;
    frame->mHeight = height;
    frame->mDisplayWidth = width;
    frame->mDisplayHeight = height;
    frame->mSize = width * height * 2;
    frame->mRotationAngle = rotation;
    frame->mData = new uint8_t[frame->mSize];

    mClient->amthumbnail_read_frame((char *)frame->mData);

#if 0
    thumbnail_decoder_close(mClient);
#endif

    return frame;
}

const char* AmlPlayerMetadataRetriever::extractMetadata(int keyCode)
{
    if (mClient == NULL) {
        ALOGV("Client is not crated !\n");
        return NULL;
    }

    if (!mParsedMetaData) {
        parseMetaData();
        mParsedMetaData = true;
    }

    ssize_t index = mMetaData.indexOfKey(keyCode);

    if (index < 0) {
        return NULL;
    }

    return strdup(mMetaData.valueAt(index).string());
}

MediaAlbumArt *AmlPlayerMetadataRetriever::extractAlbumArt()
{
    if (mClient == NULL) {
        ALOGV("Client is not crated !\n");
        return NULL;
    }

    if (!mParsedMetaData) {
        parseMetaData();

        mParsedMetaData = true;
    }

    if (mAlbumArt) {
        return mAlbumArt->clone();
    }

    return NULL;
}



static char *StringToDetector(const char *name)
{
    struct DetectorMap{
        char* from;
        char* to;
    };

    static struct DetectorMap kDetectMap[] = {
        {"album_artist", "albumartist"},
        {NULL, NULL},
    };

    struct DetectorMap *p = kDetectMap;
    while (p->from) {
        if (!strcmp(p->from, name)) {
            return p->to;
        }
        p++;
    }
    return (char *)name;
}

static char *StringFromDetector(const char *name)
{
    struct DetectorMap {
        char* from;
        char* to;
    };

    static struct DetectorMap kDetectMap[] = {
        {"albumartist", "album_artist"},
        {NULL, NULL},
    };

    struct DetectorMap *p = kDetectMap;
    while (p->from) {
        if (!strcmp(p->from, name)) {
            return p->to;
        }
        p++;
    }
    return (char *)name;
}


void AmlPlayerMetadataRetriever::parseMetaData()
{
#if 0
    if (thumbnail_find_stream_info(mClient, mFileName)) {
        ALOGV("Thumbnail find stream info failed!\n");
        return ;
    }
#endif
    int err;
    if (!mOpened) {
        err = mClient->amthumbnail_decoder_open(mFileName);
        if (err != 0) {
            ALOGV("Thumbnail decode init failed!\n");
            return;
        }

        mOpened = true;
    }

    struct Map {
        char* from;
        int      to;
    };
    static const struct Map kMap[] = {
        { "title", METADATA_KEY_TITLE },
        { "artist", METADATA_KEY_ARTIST },
        { "album", METADATA_KEY_ALBUM },
        { "genre", METADATA_KEY_GENRE },
        { "album_artist", METADATA_KEY_ALBUMARTIST },
        { "track", METADATA_KEY_CD_TRACK_NUMBER },
        { "disc", METADATA_KEY_DISC_NUMBER },
        { "composer", METADATA_KEY_COMPOSER },
        { "date", METADATA_KEY_DATE},
       // { "GPSCoordinates", METADATA_KEY_LOCATION },//old ffmpeg
        { "location", METADATA_KEY_LOCATION }, //new ffmpeg on externel
        { "rotate", METADATA_KEY_VIDEO_ROTATION },
        
        { "tracknumber", METADATA_KEY_CD_TRACK_NUMBER },
        { "discnumber", METADATA_KEY_DISC_NUMBER },
        { "year", METADATA_KEY_YEAR },
        { "duration", METADATA_KEY_DURATION },
        { "writer", METADATA_KEY_WRITER },
        { "compilation", METADATA_KEY_COMPILATION },
        { "isdrm", METADATA_KEY_IS_DRM },
        { "width", METADATA_KEY_VIDEO_WIDTH },
        { "height", METADATA_KEY_VIDEO_HEIGHT },
    };
    static const size_t kNumMapEntries = sizeof(kMap) / sizeof(kMap[0]);
    CharacterEncodingDetector *detector = new CharacterEncodingDetector();
    for (size_t i = 0; i < kNumMapEntries; ++i) {
        const char *value;
        if (mClient->amthumbnail_get_key_metadata(kMap[i].from, &value)) {
            ALOGV("get %s: %s \n", kMap[i].from, value);
            char *detectorName = StringToDetector(kMap[i].from);
            detector->addTag(detectorName, value);
        }
    }

    detector->detectAndConvert();
    int size = detector->size();
    if (size) {
        for (int m = 0; m < size; m++) {
            const char *name;
            const char *value;
            detector->getTag(m, &name, &value);
            for (size_t n = 0; n < kNumMapEntries; ++n) {
                char *detectorName = StringFromDetector(name);
                if (!strcmp(kMap[n].from, detectorName)) {
                    mMetaData.add(kMap[n].to, String8(value));
                }
            }
        }
    }
    delete detector;
    int v, a, s;
    v = a = s = 0;
    mClient->amthumbnail_get_tracks_info(&v, &a, &s);
    if (v > 0) {
        mMetaData.add(METADATA_KEY_HAS_VIDEO, String8("yes"));
    }
    if (a > 0) {
        mMetaData.add(METADATA_KEY_HAS_AUDIO, String8("yes"));
    }

    const void* data;
    int dataSize;
    if (mClient->amthumbnail_get_key_data("cover_pic", &data, &dataSize)) {
        mAlbumArt = MediaAlbumArt::fromData(dataSize, data);
    }
    int width=0,height=0;
    mClient->amthumbnail_get_video_size(&width, &height);
    if(width >0 && height>0){
        char tmp[32];
        sprintf(tmp, "%d", width);
        mMetaData.add(METADATA_KEY_VIDEO_WIDTH,String8(tmp));
        sprintf(tmp, "%d", height);
        mMetaData.add(METADATA_KEY_VIDEO_HEIGHT,String8(tmp));
    }
    char tmp[32];
    int64_t durationUs;
    mClient->amthumbnail_get_duration(&durationUs);
    sprintf(tmp, "%lld", (durationUs + 500) / 1000);
    mMetaData.add(METADATA_KEY_DURATION, String8(tmp));

#if 0
    thumbnail_find_stream_info_end(mClient);
#endif
}
}
