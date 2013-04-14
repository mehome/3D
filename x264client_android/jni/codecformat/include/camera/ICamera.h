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

#ifndef ANDROID_HARDWARE_ICAMERA_H
#define ANDROID_HARDWARE_ICAMERA_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <surfaceflinger/ISurface.h>
#include <binder/IMemory.h>
#include <utils/String8.h>
#include <camera/Camera.h>

namespace android {

class ICameraClient;

class ICamera: public IInterface
{
public:
    DECLARE_META_INTERFACE(Camera);

    virtual void            disconnect() = 0;

    // connect new client with existing camera remote
    virtual status_t        connect(const sp<ICameraClient>& client) = 0;

    // prevent other processes from using this ICamera interface
    virtual status_t        lock() = 0;

    // allow other processes to use this ICamera interface
    virtual status_t        unlock() = 0;

    // pass the buffered ISurface to the camera service
    virtual status_t        setPreviewDisplay(const sp<ISurface>& surface) = 0;

    // set the preview callback flag to affect how the received frames from
    // preview are handled.
    virtual void            setPreviewCallbackFlag(int flag) = 0;

    // start preview mode, must call setPreviewDisplay first
    virtual status_t        startPreview() = 0;

    // stop preview mode
    virtual void            stopPreview() = 0;

    // get preview state
    virtual bool            previewEnabled() = 0;

    // start recording mode
    virtual status_t        startRecording() = 0;

    // stop recording mode
    virtual void            stopRecording() = 0;    

    // get recording state
    virtual bool            recordingEnabled() = 0;

    // release a recording frame
    virtual void            releaseRecordingFrame(const sp<IMemory>& mem) = 0;

    // auto focus
    virtual status_t        autoFocus() = 0;

    // cancel auto focus
    virtual status_t        cancelAutoFocus() = 0;

    // take a picture
    virtual status_t        takePicture() = 0;

    // set preview/capture parameters - key/value pairs
    virtual status_t        setParameters(const String8& params) = 0;

    // get preview/capture parameters - key/value pairs
    virtual String8         getParameters() const = 0;

    // send command to camera driver
    virtual status_t        sendCommand(int32_t cmd, int32_t arg1, int32_t arg2) = 0;
};

// ----------------------------------------------------------------------------

class BnCamera: public BnInterface<ICamera>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif