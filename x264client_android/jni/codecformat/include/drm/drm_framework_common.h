/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef __DRM_FRAMEWORK_COMMON_H__
#define __DRM_FRAMEWORK_COMMON_H__

#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/Errors.h>

#define INVALID_VALUE -1

namespace android {

/**
 * Error code for DRM Frameowrk
 */
enum {
    DRM_ERROR_BASE = -2000,

    DRM_ERROR_UNKNOWN                       = DRM_ERROR_BASE,
    DRM_ERROR_LICENSE_EXPIRED               = DRM_ERROR_BASE - 1,
    DRM_ERROR_SESSION_NOT_OPENED            = DRM_ERROR_BASE - 2,
    DRM_ERROR_DECRYPT_UNIT_NOT_INITIALIZED  = DRM_ERROR_BASE - 3,
    DRM_ERROR_DECRYPT                       = DRM_ERROR_BASE - 4,
    DRM_ERROR_CANNOT_HANDLE                 = DRM_ERROR_BASE - 5,

    DRM_NO_ERROR                            = NO_ERROR
};

/**
 * Defines DRM Buffer
 */
class DrmBuffer {
public:
    char* data;
    int length;

    DrmBuffer() :
        data(NULL),
        length(0) {
    }

    DrmBuffer(char* dataBytes, int dataLength) :
        data(dataBytes),
        length(dataLength) {
    }

};

/**
 * Defines detailed description of the action
 */
class ActionDescription {
public:
    ActionDescription(int _outputType, int _configuration) :
        outputType(_outputType),
        configuration(_configuration) {
    }

public:
    int outputType;   /* BLUETOOTH , HDMI*/
    int configuration; /* RESOLUTION_720_480 , RECORDABLE etc.*/
};

/**
 * Defines constants related to DRM types
 */
class DrmObjectType {
private:
    DrmObjectType();

public:
    /**
     * Field specifies the unknown type
     */
    static const int UNKNOWN = 0x00;
    /**
     * Field specifies the protected content type
     */
    static const int CONTENT = 0x01;
    /**
     * Field specifies the rights information
     */
    static const int RIGHTS_OBJECT = 0x02;
    /**
     * Field specifies the trigger information
     */
    static const int TRIGGER_OBJECT = 0x03;
};

/**
 * Defines constants related to play back
 */
class Playback {
private:
    Playback();

public:
    /**
     * Constant field signifies playback start
     */
    static const int START = 0x00;
    /**
     * Constant field signifies playback stop
     */
    static const int STOP = 0x01;
    /**
     * Constant field signifies playback paused
     */
    static const int PAUSE = 0x02;
    /**
     * Constant field signifies playback resumed
     */
    static const int RESUME = 0x03;
};

/**
 * Defines actions that can be performed on protected content
 */
class Action {
private:
    Action();

public:
    /**
     * Constant field signifies that the default action
     */
    //static const int DEFAULT = 0x00;
    /**
     * Constant field signifies that the content can be played
     */
    //static const int PLAY = 0x01;
    /**
     * Constant field signifies that the content can be set as ring tone
     */
    //static const int RINGTONE = 0x02;
    /**
     * Constant field signifies that the content can be transfered
     */
    //static const int TRANSFER = 0x03;
    /**
     * Constant field signifies that the content can be set as output
     */
    //static const int OUTPUT = 0x04;
    /**
     * Constant field signifies that preview is allowed
     */
    //static const int PREVIEW = 0x05;
    /**
     * Constant field signifies that the content can be executed
     */
    //static const int EXECUTE = 0x06;
    /**
     * Constant field signifies that the content can displayed
     */
    //static const int DISPLAY = 0x07;

    static const int PLAY = 0x01;
    static const int DISPLAY = 0x02;
    static const int EXECUTE = 0x04;
    static const int PRINT = 0x08;

    // FL has ringtone permission, CD/SD don't
    static const int RINGTONE = 0x10;
    // SD has transfer permission, FL/CD don't
    static const int TRANSFER = 0x20;
    // FL has wallpaper permission, CD/SD don't
    static const int WALLPAPER = 0x40;
};

/**
 * Defines constants related to status of the rights
 */
class RightsStatus {
private:
    RightsStatus();

public:
    /**
     * Constant field signifies that the rights are valid
     */
    static const int RIGHTS_VALID = 0x00;
    /**
     * Constant field signifies that the rights are invalid
     */
    static const int RIGHTS_INVALID = 0x01;
    /**
     * Constant field signifies that the rights are expired for the content
     */
    static const int RIGHTS_EXPIRED = 0x02;
    /**
     * Constant field signifies that the rights are not acquired for the content
     */
    static const int RIGHTS_NOT_ACQUIRED = 0x03;
    /**
     * Constant field signifies that the secure timer invalid
     */
    static const int SECURE_TIMER_INVALID = 0x04;
};

/**
 * Defines API set for decryption
 */
class DecryptApiType {
private:
    DecryptApiType();

public:
    /**
     * Decrypt API set for non encrypted content
     */
    static const int NON_ENCRYPTED = 0x00;
    /**
     * Decrypt API set for ES based DRM
     */
    static const int ELEMENTARY_STREAM_BASED = 0x01;
    /**
     * POSIX based Decrypt API set for container based DRM
     */
    static const int CONTAINER_BASED = 0x02;
};

/**
 * Defines decryption information
 */
class DecryptInfo {
public:
    /**
     * size of memory to be allocated to get the decrypted content.
     */
    int decryptBufferLength;
    /**
     * reserved for future purpose
     */
};

/**
 * Defines decryption handle
 */
class DecryptHandle {
public:
    /**
     * Decryption session Handle
     */
    int decryptId;
    /**
     * Mimetype of the content to be used to select the media extractor
     * For e.g., "video/mpeg" or "audio/mp3"
     */
    String8 mimeType;
    /**
     * Defines which decryption pattern should be used to decrypt the given content
     * DrmFramework provides two different set of decryption APIs.
     *   1. Decrypt APIs for elementary stream based DRM
     *      (file format is not encrypted but ES is encrypted)
     *         e.g., Marlin DRM (MP4 file format), WM-DRM (asf file format)
     *
     *         DecryptApiType::ELEMENTARY_STREAM_BASED
     *             Decryption API set for ES based DRM
     *                 initializeDecryptUnit(), decrypt(), and finalizeDecryptUnit()
     *   2. Decrypt APIs for container based DRM (file format itself is encrypted)
     *         e.g., OMA DRM (dcf file format)
     *
     *         DecryptApiType::CONTAINER_BASED
     *             POSIX based Decryption API set for container based DRM
     *                 pread()
     */
    int decryptApiType;
    /**
     * Defines the status of the rights like
     *     RIGHTS_VALID, RIGHTS_INVALID, RIGHTS_EXPIRED or RIGHTS_NOT_ACQUIRED
     */
    int status;
    /**
     * Information required to decrypt content
     * e.g. size of memory to be allocated to get the decrypted content.
     */
    DecryptInfo* decryptInfo;

public:
    DecryptHandle():
            decryptId(INVALID_VALUE),
            mimeType(""),
            decryptApiType(INVALID_VALUE),
            status(INVALID_VALUE) {

    }

    bool operator<(const DecryptHandle& handle) const {
        return (decryptId < handle.decryptId);
    }

    bool operator==(const DecryptHandle& handle) const {
        return (decryptId == handle.decryptId);
    }
};

};

#endif /* __DRM_FRAMEWORK_COMMON_H__ */
