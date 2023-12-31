/*
 * Copyright (c) 2013-2018 TRUSTONIC LIMITED
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

#ifndef MCP_H_
#define MCP_H_


#include "mcUuid.h"
#include "mcLoadFormat.h"
#include "mcVersionInfo.h"
#include "stdbool.h"

#define FLAG_RESPONSE       (1U << 31)  /**< Flag to indicate that this is the response to a MCP command. */

/** MobiCore Return Code Defines.
 * List of the possible MobiCore return codes.
 */
typedef enum {
    MC_MCP_RET_OK                                   =  0, /**< Memory has successfully been mapped. */
    MC_MCP_RET_ERR_INVALID_SESSION                  =  1, /**< The session ID is invalid. */
    MC_MCP_RET_ERR_UNKNOWN_UUID                     =  2, /**< The UUID of the Trustlet is unknown. */
    MC_MCP_RET_ERR_UNKNOWN_DRIVER_ID                =  3, /**< The ID of the driver is unknown. */
    MC_MCP_RET_ERR_NO_MORE_SESSIONS                 =  4, /**< No more session are allowed. */
    MC_MCP_RET_ERR_TRUSTLET_INVALID                 =  6, /**< The Trustlet is invalid. */
    MC_MCP_RET_ERR_ALREADY_MAPPED                   =  7, /**< The memory block has already been mapped before. */
    MC_MCP_RET_ERR_INVALID_PARAM                    =  8, /**< Alignment or length error in the command parameters. */
    MC_MCP_RET_ERR_OUT_OF_RESOURCES                 =  9, /**< No space left in the virtual address space of the session. */
    MC_MCP_RET_ERR_INVALID_WSM                      = 10, /**< WSM type unknown or broken WSM */
    MC_MCP_RET_ERR_UNKNOWN                          = 11, /**< unknown error. */
    MC_MCP_RET_ERR_INVALID_MAPPING_LENGTH           = 12, /**< Lenght of map invalid */
    MC_MCP_RET_ERR_MAPPING_TARGET                   = 13, /**< Map can only be applied to Trustlet session */
    MC_MCP_RET_ERR_OUT_OF_CRYPTO_RESSOURCES         = 14, /**< Couldn't open crypto session. */
    MC_MCP_RET_ERR_SIGNATURE_VERIFICATION_FAILED    = 15, /**< System Trustlet signature verification failed. */
    MC_MCP_RET_ERR_WRONG_PUBLIC_KEY                 = 16, /**< System Trustlet public key is wrong. */
    MC_MCP_RET_ERR_SP_TL_HASH_CHECK_FAILED          = 26, /**< Hash check of service provider trustlet failed. */
    MC_MCP_RET_ERR_LAUNCH_TASK_FAILED               = 27, /**< Activation/starting of task failed. */
    MC_MCP_RET_ERR_CLOSE_TASK_FAILED                = 28, /**< Closing of task not yet possible, try again later. */
    MC_MCP_RET_ERR_SERVICE_BLOCKED                  = 29, /**< Service is blocked and a session cannot be opened to it. */
    MC_MCP_RET_ERR_SERVICE_LOCKED                   = 30, /**< Service is locked and a session cannot be opened to it. */
    MC_MCP_RET_ERR_DOWNGRADE_NOT_AUTHORIZED         = 32, /**< Service version is lower than the one installed. */
    MC_MCP_RET_ERR_SYSTEM_NOT_READY                 = 33, /**< Filesystem not yet ready. */

    // used for command verification
    MC_MCP_RET_ERR_UNKNOWN_COMMAND                  = 50, /**< The command is unknown. */
    MC_MCP_RET_ERR_INVALID_DATA                     = 51, /**< The command data is invalid. */
    MC_MCP_RET_ERR_SECURITY                         = 52  /**< security violation such as brute force attack detection. */
} mcpResult_t;

/** Possible MCP Command IDs
 * Command ID must be between 0 and 0x7FFFFFFF.
 */
typedef enum {
    MC_MCP_CMD_ID_INVALID                = 0x00000000,   /**< Invalid command ID. */
    // Session commands
    MC_MCP_CMD_OPEN_SESSION              = 0x00000001,   /**< Open a session to a service. */
    MC_MCP_CMD_CLOSE_SESSION             = 0x00000003,   /**< Close an existing service session. */
    MC_MCP_CMD_MAP                       = 0x00000004,   /**< Map a block of WSM to a session. */
    MC_MCP_CMD_UNMAP                     = 0x00000005,   /**< Unmap a block of WSM from a session. */
    // obsolete: MC_MCP_CMD_SUSPEND      = 0x00000006,
    // obsolete: MC_MCP_CMD_RESUME       = 0x00000007,
    // obsolete: MC_MCP_CMD_DONATE_RAM   = 0x00000008,
    MC_MCP_CMD_GET_MOBICORE_VERSION      = 0x00000009,   /**< Get MobiCore version information. */
    MC_MCP_CMD_CLOSE_MCP                 = 0x0000000A,   /**< Close MCP and unmap MCI. */
    MC_MCP_CMD_LOAD_TOKEN                = 0x0000000B,   /**< Load token for device attestation */
    // obsolete: MC_MCP_CMD_CHECK_LOAD_TA = 0x0000000C,
    MC_MCP_CMD_LOAD_SYSENC_KEY_SO        = 0x0000000D,   /**< Load a decryption key */

} mcpCmdId_t;


/** Types of WSM known to the MobiCore.
 */
typedef uint32_t wsmType_t;

#define WSM_TYPE_MASK       0xFF
#define WSM_INVALID         0       /**< Invalid memory type */
#define WSM_CONTIGUOUS      1       /**< Reference to WSM points to a contiguous region of pages. */
#define WSM_L2_OBSOLETE     2       /**< This Flag is not used anymore */
#define WSM_L1              3       /**< Reference to WSM points to an L1 table describing the memory region to share */
#define WSM_UNCACHED        0x100   /**< Bitflag indicating that memory should be uncached */

/** Magic number used to identify if Open Command supports GP client authentication. */
#define MC_GP_CLIENT_AUTH_MAGIC    0x47504131  /* "GPA1" */

/** Initialisation values flags. */
#define MC_IV_FLAG_IRQ              (1 << 0)    /* Set if IRQ is present */
#define MC_IV_FLAG_TIME             (1 << 1)    /* Set if GP TIME API is supported */
#define MC_IV_FLAG_IWP              (1 << 2)    /* Set if GP client uses interworld session */

typedef struct {
    uint32_t flags;  /**< Flags to know what data to expect. */
    uint32_t irq;    /**< IRQ number to use. */
    uint32_t timeOffset;/**< Offset from MCI base address of mcTime struct. */
    uint32_t timeLen;/**< Length of mcTime struct. */
    uint32_t iws_buf_ofs;   /* interworld session buffer offset in MCI */
    uint32_t iws_buf_size;  /* interworld session buffer size */
    uint8_t  padding[8];
} initValues_t;

/** Command header.
 * It just contains the command ID. Only values specified in mcpCmdId_t are allowed as command IDs.
 * If the command ID is unspecified the MobiCore returns an empty response with the result set to MC_MCP_RET_ERR_UNKNOWN_COMMAND .
 */
typedef struct {
    mcpCmdId_t cmdId; /**< Command ID of the command */
} commandHeader_t, *commandHeader_ptr;

/** Response header.
 * MobiCore will reply to every MCP command with an MCP response. Like the MCP command the response consists of a
 * header followed by response data. The response is written to the same memory location as the MCP command.
 */
typedef struct {
    uint32_t rspId;  /**< Command ID | FLAG_RESPONSE. */
    mcpResult_t result; /**< Result informs about the execution result of the command associated with the response. */
} responseHeader_t, *responseHeader_ptr;



/** @defgroup CMD MCP Commands
 * @{ */

/** @defgroup ASMCMD Administrative Commands
 * @{ */

/** @defgroup MCPGETMOBICOREVERSION GET_MOBICORE_VERSION
 * Get MobiCore version info.
 *
 * @{ */

/** Get MobiCore Version Command. */
typedef struct {
    commandHeader_t cmdHeader;  /** Command header. */
} mcpCmdGetMobiCoreVersion_t, *mcpCmdGetMobiCoreVersion_ptr;

/** Get MobiCore Version Command Response. */
typedef struct {
    responseHeader_t rspHeader;   /** Response header. */
    mcVersionInfo_t  versionInfo; /** MobiCore version info. */
} mcpRspGetMobiCoreVersion_t, *mcpRspGetMobiCoreVersion_ptr;

/** @} */// End MCPGETMOBICOREVERSION

/** @} */// End ASMCMD

/** @defgroup SESSCMD Session Management Commands
 * @{ */

/** @defgroup MCPOPEN OPEN
 * Load and open a session to a Trustlet.
 * The OPEN command loads Trustlet data to the MobiCore context and opens a session to the Trustlet.
 * If wsmTypeLoadData is WSM_INVALID MobiCore tries to start a pre-installed Trustlet
 * associated with the uuid passed.
 * The uuid passed must match the uuid contained in the load data (if available).
 * On success, MobiCore returns the session ID which can be used for further communication.
 * @{ */

/** GP client authentication data */
typedef struct {
    uint32_t          mclfMagic;        /**< ASCII "GPA1" if GP client authentication is supported,
                                                   "MCLF" otherwise. */
    mcIdentity_t      mcIdentity;       /**< GP identity information: login method and data */
} cmdOpenData_t;

/** Open Command */
typedef struct {
    commandHeader_t   cmdHeader;        /**< Command header. */
    mcUuid_t          uuid;             /**< Byte array containing the service UUID. */
    uint8_t           pad1[4];          /**< Padding to be 64-bit aligned */
    uint64_t          adrTciBuffer;     /**< Physical address of the TCI */
    uint64_t          adrLoadData;      /**< Physical address of the data to load. */
    uint32_t          ofsTciBuffer;     /**< Offset to the data. */
    uint32_t          lenTciBuffer;     /**< Length of the TCI. */
    wsmType_t         wsmTypeTci;       /**< Type of WSM used for the TCI */
    wsmType_t         wsmTypeLoadData;  /**< Type of the memory containing the data to load. */
    uint32_t          ofsLoadData;      /**< Offset to the data. */
    uint32_t          lenLoadData;      /**< Length of the data to load. */
    union {
        cmdOpenData_t cmdOpenData;      /**< Magic number and login info for GP client authentication. */
        mclfHeader_t  tlHeader;         /**< Service header */
    };
    uint32_t          unused;           /**< Check on is_gpta is now unused */
} mcpCmdOpen_t, *mcpCmdOpen_ptr;

/** Open Command Response */
typedef struct {
    responseHeader_t  rspHeader; /**< Response header. */
    uint32_t          sessionId; /**< Session ID used for further communication. */
} mcpRspOpen_t, *mcpRspOpen_ptr;

/** @} */// End MCPOPEN


/** @defgroup MCPCLOSE CLOSE
 * Close an existing session to a Trustlet.
 * The CLOSE command terminates a session and frees all resources in the MobiCore system which
 * are currently occupied by the session. Before closing the session, the MobiCore runtime
 * management waits until all pending operations, like calls to drivers, invoked by the Trustlet
 * have been terminated.
 * Mapped memory will automatically be unmapped from the MobiCore context. The NWd is responsible for
 * processing the freed memory according to the Rich-OS needs.
 *
 * @{ */

/** Close Command */
typedef struct {
    commandHeader_t  cmdHeader;  /**< Command header. */
    uint32_t         sessionId;  /**< Session ID. */
} mcpCmdClose_t, *mcpCmdClose_ptr;

/** Close Command Response */
typedef struct {
    responseHeader_t  rspHeader; /**< Response header. */
} mcpRspClose_t, *mcpRspClose_ptr;

/** @} */// End MCPCLOSE


/** @defgroup MCPMAP MAP
 * Map a portion of memory to a session.
 * The MAP command provides a block of memory to the context of a service.
 * The memory then becomes world-shared memory (WSM).
 * The only allowed memory type here is WSM_L1.
 * @{ */

/** Map Command */
typedef struct {
    commandHeader_t  cmdHeader;     /**< Command header. */
    uint32_t         sessionId;     /**< Session ID of a valid session */
    wsmType_t        wsmType;       /**< Type of WSM used of the memory*/
    uint32_t         ofsBuffer;     /**< Offset to the payload. */
    uint64_t         adrBuffer;     /**< Physical address of the memory */
    uint32_t         lenBuffer;     /**< Length of the buffer. */
    uint32_t         flags;         /**< Attribute of the buffer (read/write) */
} mcpCmdMap_t, *mcpCmdMap_ptr;

#define MCP_MAP_MAX         0x100000    /**< Maximum allowed length for MCP map. */

/** Map Command Response */
typedef struct {
    responseHeader_t  rspHeader;        /**< Response header. */
    uint32_t          secureVirtualAdr; /**< Virtual address in the context of the service the WSM is mapped to, already includes a possible offset! */
} mcpRspMap_t, *mcpRspMap_ptr;

/** @} *///End MCPMAP


/** @defgroup MCPUNMAP UNMAP
 * Unmap a portion of world-shared memory from a session.
 * The UNMAP command is used to unmap a previously mapped block of
 * world shared memory from the context of a session.
 *
 * Attention: The memory block will be immediately unmapped from the specified session.
 * If the service is still accessing the memory, the service will trigger a segmentation fault.
 * @{ */

/** Unmap Command */
typedef struct {
    commandHeader_t  cmdHeader;         /**< Command header. */
    uint32_t         sessionId;         /**< Session ID of a valid session */
    wsmType_t        wsmType;           /**< Type of WSM used of the memory*/
    uint32_t         secureVirtualAdr;  /**< Virtual address in the context of the service the WSM has been mapped to, already includes a possible offset! */
    uint32_t         lenVirtualBuffer;  /**< Length of the virtual buffer. */
} mcpCmdUnmap_t, *mcpCmdUnmap_ptr;

/** Unmap Command Response */
typedef struct {
    responseHeader_t rspHeader; /**< Response header. */
} mcpRspUnmap_t, *mcpRspUnmap_ptr;

/** @} */// End MCPUNMAP

/** @} */// End SESSCMD

/** @defgroup MCPLOADTOKEN LOADTOKEN
 * Load a token from the normal world and share it with Kinibi
 * If something fails, the device attestation functionality will be disabled
 * @{ */

/** Load Token */
typedef struct {
    commandHeader_t   cmdHeader;        /**< Command header. */
    wsmType_t         wsmTypeLoadData;  /**< Type of the memory containing the data to load. */
    uint64_t          adrLoadData;      /**< Physical address of the data to load. */
    uint64_t          ofsLoadData;      /**< Offset to the data to load. */
    uint64_t          lenLoadData;      /**< Length of the data to load. */
} mcpCmdLoadToken_t, *mcpCmdLoadToken_ptr;

/** Load Token Command Response */
typedef struct {
    responseHeader_t  rspHeader; /**< Response header. */
} mcpRspLoadToken_t, *mcpRspLoadToken_ptr;

/** @} *///End MCPLOADTOKEN

/** @defgroup MCPLOADKEYSO LOADKEYSO
 * Load a key SO from the normal world and share it with Kinibi
 * If something fails, the system service decryption functionality will be disabled
 * @{ */

/** Load Key SO */
typedef struct {
    commandHeader_t   cmdHeader;        /**< Command header. */
    wsmType_t         wsmTypeLoadData;  /**< Type of the memory containing the data to load. */
    uint64_t          adrLoadData;      /**< Physical address of the data to load. */
    uint64_t          ofsLoadData;      /**< Offset to the data to load. */
    uint64_t          lenLoadData;      /**< Length of the data to load. */
} mcpCmdLoadKeySo_t, *mcpCmdLoadKeySo_ptr;

/** Load Key SO Command Response */
typedef struct {
    responseHeader_t  rspHeader; /**< Response header. */
} mcpRspLoadKeySo_t, *mcpRspLoadKeySo_ptr;

/** @} *///End MCPLOADKEYSO

/** @} */// End CMD

/** Structure of the MCP buffer. */
typedef union {
    initValues_t                 initValues;             /**< initialization values. */
    commandHeader_t              cmdHeader;              /**< Command header. */
    responseHeader_t             rspHeader;              /**< Response header. */
    mcpCmdOpen_t                 cmdOpen;                /**< Load and open service. */
    mcpRspOpen_t                 rspOpen;                /**< Response to load and open service. */
    mcpCmdClose_t                cmdClose;               /**< Close command. */
    mcpRspClose_t                rspClose;               /**< Response to close command. */
    mcpCmdMap_t                  cmdMap;                 /**< Map WSM to service context. */
    mcpRspMap_t                  rspMap;                 /**< Response to MAP command. */
    mcpCmdUnmap_t                cmdUnmap;               /**< Unmap WSM from service context. */
    mcpRspUnmap_t                rspUnmap;               /**< Response to UNMAP command. */
    mcpCmdGetMobiCoreVersion_t   cmdGetMobiCoreVersion;  /**< Get MobiCore Version command. */
    mcpRspGetMobiCoreVersion_t   rspGetMobiCoreVersion;  /**< Response to GET_MOBICORE_VERSION command. */
    mcpCmdLoadToken_t            cmdLoadToken;
    mcpRspLoadToken_t            rspLoadToken;
    mcpCmdLoadKeySo_t            mcpCmdLoadKeySo;
    mcpRspLoadKeySo_t            mcpRspLoadKeySo;
} mcpMessage_t, *mcpMessage_ptr;

#define MC_STATE_FLAG_TEE_HALT_MASK (1U << 0)

/** MobiCore status flags */
typedef struct {
    uint16_t      RFU1;             /**< Formerly schedule hint, now reserved */
    uint16_t      required_workers; /**< Swd requested nb of TEE workers */
    uint32_t      RFU2;             /**< Kept for backward compatibility with the NWd */
    uint32_t      timeout;          /**< Secure-world timeout: when Kinibi goes to sleep, its next deadline is written here */
    uint8_t       tee_flags;        /**< TEE flags */
    uint8_t       RFU_padding[3];   /**< Reserved for future use */
} mcFlags_t, *mcFlags_ptr;

/** MCP buffer structure */
typedef struct {
    mcFlags_t     mcFlags;    /**< MobiCore Flags */
    mcpMessage_t  mcpMessage; /**< MCP message buffer */
} mcpBuffer_t, *mcpBuffer_ptr;

/** @} */
#endif /* MCP_H_ */
