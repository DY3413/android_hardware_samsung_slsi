/*
 * Copyright (c) 2013-2016 TRUSTONIC LIMITED
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the TRUSTONIC LIMITED nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __gatekeeper_tci_h__
#define __gatekeeper_tci_h__

#include "def.h"
#include "mcSo.h"
#include "ta_hw_auth_token.h"
#include "ta_password_handle.h"
#include "gatekeeper_version.h"

/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)

#ifdef __cplusplus
typedef gatekeeper::password_handle_t password_handle_t;
#else
typedef struct password_handle_t password_handle_t;
#endif

typedef uint32_t tciCommandId_t;
typedef uint32_t tciResponseId_t;

typedef uint64_t timestamp_t;

/** Android's user ID */
typedef uint32_t user_id_t;

/**
 * Only those error codes are returned by the Gatekeeper TA.
 */
typedef enum {
    ERROR_NONE = 0,
    ERROR_INVALID = 1,
    ERROR_RETRY = 2,
    ERROR_UNKNOWN = 3,
} gatekeeper_error_t;

/***
 * Used for sending blob bewtween NWd and SWd
 */
typedef struct {
    uint32_t data;          /* 32 bits secure address */
    uint32_t data_length;   /* data length */
} data_blob_t;

typedef struct __attribute__ ((__packed__)) {
    uint32_t        uid;
    data_blob_t     current_password;
    data_blob_t     current_password_handle;
    data_blob_t     desired_password;
} enroll_request_t;

typedef struct __attribute__ ((__packed__)) {
    password_handle_t   password_handle;
    int                 throttle_ms;
} enroll_response_t;

/**
 * Request structure used for passing UID to be deleted
 */
typedef struct __attribute__ ((__packed__)) {
    user_id_t   uid;            // UID of failure record to be deleted. Ignored if delete_all = true
    bool        delete_all;     // If delete_all = true all failure records will be deleted
} delete_user_request_t;

typedef struct __attribute__ ((__packed__)) {
    uint32_t        uid;
    uint64_t        challenge;
    data_blob_t     enrolled_password_handle;
    data_blob_t     provided_password;
} verify_request_t;

typedef struct __attribute__ ((__packed__)) {
    bool                request_reenroll;
    int                 throttle_ms;
    hw_auth_token_t     auth_token;
} verify_response_t;

typedef struct __attribute__ ((__packed__)) {
    gatekeeper_version_t    version;
} get_version_t;

/**
 * TCI command header.
 */
typedef struct {
    tciCommandId_t commandId; /**< Command ID */
} tciCommandHeader_t;

/**
 * TCI response header.
 */
typedef struct {
    tciResponseId_t     responseId; /**< Response ID (must be command ID | RSP_ID_MASK )*/
    gatekeeper_error_t  returnCode; /**< Return code of command */
} tciResponseHeader_t;

/**
 * Response structure
 */
typedef struct {
    tciResponseHeader_t header; /**< Response header */
    uint32_t len;
} response_t;


/**
 * Command message.
 *
 * @param len Length of the data to proces,s.
 * @param data Data to be processed
 */
typedef struct {
    tciCommandHeader_t header; /**< Command header */
    uint32_t len; /**< Length of data to process */
} command_t;

/**
 * TCI message data.
 */
typedef struct {
    union {
        command_t command;
        response_t response;
    };

    union {
        enroll_request_t      enroll_request;
        enroll_response_t     enroll_response;
        delete_user_request_t delete_user_request;
        verify_request_t      verify_request;
        verify_response_t     verify_response;
        get_version_t         get_version;
    };

} tciMessage_t, *tciMessage_ptr;

/**
 * Overall TCI structure.
 */
typedef struct {
    tciMessage_t message; /**< TCI message */
} tci_t;

#endif // __gatekeeper_tci_h__
