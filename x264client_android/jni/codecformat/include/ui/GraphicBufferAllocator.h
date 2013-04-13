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
**
** Copyright 2009, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef ANDROID_BUFFER_ALLOCATOR_H
#define ANDROID_BUFFER_ALLOCATOR_H

#include <stdint.h>

#include <cutils/native_handle.h>

#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/threads.h>
#include <utils/Singleton.h>

#include <ui/PixelFormat.h>

#include <hardware/gralloc.h>


namespace android {
// ---------------------------------------------------------------------------

class String8;

class GraphicBufferAllocator : public Singleton<GraphicBufferAllocator>
{
public:
    enum {
        USAGE_SW_READ_NEVER     = GRALLOC_USAGE_SW_READ_NEVER,
        USAGE_SW_READ_RARELY    = GRALLOC_USAGE_SW_READ_RARELY,
        USAGE_SW_READ_OFTEN     = GRALLOC_USAGE_SW_READ_OFTEN,
        USAGE_SW_READ_MASK      = GRALLOC_USAGE_SW_READ_MASK,
        
        USAGE_SW_WRITE_NEVER    = GRALLOC_USAGE_SW_WRITE_NEVER,
        USAGE_SW_WRITE_RARELY   = GRALLOC_USAGE_SW_WRITE_RARELY,
        USAGE_SW_WRITE_OFTEN    = GRALLOC_USAGE_SW_WRITE_OFTEN,
        USAGE_SW_WRITE_MASK     = GRALLOC_USAGE_SW_WRITE_MASK,
        
        USAGE_SOFTWARE_MASK     = USAGE_SW_READ_MASK|USAGE_SW_WRITE_MASK,
        
        USAGE_HW_TEXTURE        = GRALLOC_USAGE_HW_TEXTURE,
        USAGE_HW_RENDER         = GRALLOC_USAGE_HW_RENDER,
        USAGE_HW_2D             = GRALLOC_USAGE_HW_2D,
        USAGE_HW_MASK           = GRALLOC_USAGE_HW_MASK
    };

    static inline GraphicBufferAllocator& get() { return getInstance(); }
    

    status_t alloc(uint32_t w, uint32_t h, PixelFormat format, int usage,
            buffer_handle_t* handle, int32_t* stride);

    status_t free(buffer_handle_t handle);

    void dump(String8& res) const;
    static void dumpToSystemLog();

private:
    struct alloc_rec_t {
        uint32_t w;
        uint32_t h;
        uint32_t s;
        PixelFormat format;
        uint32_t usage;
        size_t size;
    };
    
    static Mutex sLock;
    static KeyedVector<buffer_handle_t, alloc_rec_t> sAllocList;
    
    friend class Singleton<GraphicBufferAllocator>;
    GraphicBufferAllocator();
    ~GraphicBufferAllocator();
    
    alloc_device_t  *mAllocDev;
};

// ---------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_BUFFER_ALLOCATOR_H
