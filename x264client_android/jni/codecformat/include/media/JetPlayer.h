/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef JETPLAYER_H_
#define JETPLAYER_H_

#include <utils/threads.h>
#include <nativehelper/jni.h>

#include <libsonivox/jet.h>
#include <libsonivox/eas_types.h>
#include "AudioTrack.h"


namespace android {

typedef void (*jetevent_callback)(int eventType, int val1, int val2, void *cookie);

class JetPlayer {

public:

    // to keep in sync with the JetPlayer class constants
    // defined in frameworks/base/media/java/android/media/JetPlayer.java
    static const int JET_EVENT                   = 1;
    static const int JET_USERID_UPDATE           = 2;
    static const int JET_NUMQUEUEDSEGMENT_UPDATE = 3;
    static const int JET_PAUSE_UPDATE            = 4;

    JetPlayer(jobject javaJetPlayer, 
            int maxTracks = 32, 
            int trackBufferSize = 1200);
    ~JetPlayer();
    int init();
    int release();
    
    int loadFromFile(const char* url);
    int loadFromFD(const int fd, const long long offset, const long long length);
    int closeFile();
    int play();
    int pause();
    int queueSegment(int segmentNum, int libNum, int repeatCount, int transpose,
            EAS_U32 muteFlags, EAS_U8 userID);
    int setMuteFlags(EAS_U32 muteFlags, bool sync);
    int setMuteFlag(int trackNum, bool muteFlag, bool sync);
    int triggerClip(int clipId);
    int clearQueue();

    void setEventCallback(jetevent_callback callback);
    
    int getMaxTracks() { return mMaxTracks; };


private:
    static  int         renderThread(void*);
    int                 render();
    void                fireUpdateOnStatusChange();
    void                fireEventsFromJetQueue();

    JetPlayer() {} // no default constructor
    void dump();
    void dumpJetStatus(S_JET_STATUS* pJetStatus);

    jetevent_callback   mEventCallback;

    jobject             mJavaJetPlayerRef;
    Mutex               mMutex; // mutex to sync the render and playback thread with the JET calls
    pid_t               mTid;
    Condition           mCondition;
    volatile bool       mRender;
    bool                mPaused;

    EAS_STATE           mState;
    int*                mMemFailedVar;

    int                 mMaxTracks; // max number of MIDI tracks, usually 32
    EAS_DATA_HANDLE     mEasData;
    EAS_FILE_LOCATOR    mEasJetFileLoc;
    EAS_PCM*            mAudioBuffer;// EAS renders the MIDI data into this buffer, 
    AudioTrack*         mAudioTrack; // and we play it in this audio track
    int                 mTrackBufferSize;
    S_JET_STATUS        mJetStatus;
    S_JET_STATUS        mPreviousJetStatus;

    char                mJetFilePath[256];


}; // end class JetPlayer

} // end namespace android



#endif /*JETPLAYER_H_*/
