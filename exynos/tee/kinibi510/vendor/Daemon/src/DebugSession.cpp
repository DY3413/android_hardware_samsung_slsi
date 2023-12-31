/*
 * Copyright (c) 2015-2020 TRUSTONIC LIMITED
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

/**
 * DebugSession server.
 *
 * Maintains a session to SWd and dumps session information into dump file when
 * signal USR2 is received.
 */

#include <memory>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "dynamic_log.h"

#include "MobiCoreDriverApi.h"
#include "mcdebugsession.h" // access the size offset in the tci
#include "DebugSession.h"

#define DEFAULT_DEBUG_SESSION_SIZE (4 * 1024)

#ifdef ANDROID
/* ExySp */
#define DUMP_FILE "/data/vendor/log/debugsession"
#else
#define DUMP_FILE "/tmp/debugsession"
#endif

struct DebugSession::Impl {
    mcSessionHandle_t    session_handle = { 0, 0 };
    bool                 session_open = false;
    std::unique_ptr<Tci> tci;

    SecureWorld* secure_world;
    Impl(SecureWorld* sw): secure_world(sw) {}
};

DebugSession::DebugSession(SecureWorld* p_secure_world):
    pimpl_(new Impl(p_secure_world)) {}

DebugSession::~DebugSession() {
    delete pimpl_;
}

int DebugSession::open() {
    mcResult_t mcRet;
    const mcUuid_t uuid = MC_UUID_DEBUG_SESSION;

    // Open device
    mcRet = mcOpenDevice(MC_DEVICE_ID_DEFAULT);
    if (MC_DRV_OK != mcRet) {
        LOG_E("mcOpenDevice returned: 0x%08X\n", mcRet);
        return -1;
    }

    // Open session
    ::memset(&pimpl_->session_handle, 0, sizeof(pimpl_->session_handle));
    pimpl_->session_handle.deviceId = MC_DEVICE_ID_DEFAULT;
    pimpl_->tci.reset(new Tci(DEFAULT_DEBUG_SESSION_SIZE));
    mcRet = pimpl_->tci->alloc();
    if (MC_DRV_OK != mcRet) {
        LOG_E("mcMallocWsm returned: 0x%08X\n", mcRet);
        mcCloseDevice(MC_DEVICE_ID_DEFAULT);
        return -1;
    }

    mcRet = mcOpenSession(&pimpl_->session_handle,
                          &uuid,
                          pimpl_->tci->address(),
                          pimpl_->tci->length());
    if (MC_DRV_ERR_INVALID_OPERATION == mcRet) { //TODO verify
        // The tci can be too short, the SWd has maj the ideal tci size in the
        // corresponding tci buffer
        auto header = reinterpret_cast<const debugsession_header_t*>(pimpl_->tci->address());
        LOG_W("Tci buffer might be too short, trying again with new size sent by SWd (size = %u bytes)",
              header->size);

        size_t tci_len = header->size;
        pimpl_->tci.reset(new Tci(tci_len));
        mcRet = pimpl_->tci->alloc();
        if (MC_DRV_OK != mcRet) {
            LOG_E("mcMallocWsm returned: 0x%08X\n", mcRet);
            mcCloseDevice(MC_DEVICE_ID_DEFAULT);
            return -1;
        }

        // Try to open the session with new size
        mcRet = mcOpenSession(&pimpl_->session_handle,
                              &uuid,
                              pimpl_->tci->address(),
                              pimpl_->tci->length());
        if (MC_DRV_OK != mcRet) {
            LOG_E("Can't open session after two attempts");
            pimpl_->tci.reset();
            mcCloseDevice(MC_DEVICE_ID_DEFAULT);
            return -1;
        }
    } else if (MC_DRV_OK != mcRet) {
        LOG_E("mcOpenSession returned: 0x%08X\n", mcRet);
        pimpl_->tci.reset();
        mcCloseDevice(MC_DEVICE_ID_DEFAULT);
        return -1;
    }

    pimpl_->secure_world->registerTeeDeathCallback(std::bind(&DebugSession::dump, this));

    pimpl_->session_open = true;
    LOG_I("session opened");
    return 0;
}

void DebugSession::close() {
    mcResult_t mcRet;

    // Close session
    mcRet = mcCloseSession(&pimpl_->session_handle);
    if (MC_DRV_OK != mcRet) {
        LOG_E("%s: mcCloseSession returned: 0x%08X", __func__, mcRet);
    }
    memset(&pimpl_->session_handle, 0, sizeof(mcSessionHandle_t));
    pimpl_->session_open = false;
    pimpl_->tci.reset();

    // Close device
    mcRet = mcCloseDevice(MC_DEVICE_ID_DEFAULT);
    if (MC_DRV_OK != mcRet) {
        LOG_E("%s: mcCloseDevice returned: 0x%08X", __func__, mcRet);
    }

    LOG_I("closed");
}

void DebugSession::receiveSignal(int signum) {
    if ((signum == SIGUSR2) || (signum == 0)) {
        dump();
    }
}

int DebugSession::dump() {
    if (!pimpl_->session_open) {
        LOG_E("Can't dump the info page: the session isn't open");
        return -1;
    }

    std::string path = DUMP_FILE;
    FILE* fd = ::fopen(path.c_str(), "w");
    if (fd == NULL) {
        path += " open";
        LOG_ERRNO(path.c_str());
        return -1;
    }

    size_t res = ::fwrite(pimpl_->tci->address(), 1, pimpl_->tci->length(), fd);
    if (res != pimpl_->tci->length()) {
        path += " write";
        LOG_ERRNO(path.c_str());
        ::fclose(fd);
        return -1;
    }

    if (::fclose(fd)) {
        path += " close";
        LOG_ERRNO(path.c_str());
        return -1;
    }

    LOG_I("Info page dumped to %s", path.c_str());
    return 0;
}

//------------------------------------------------------------------------------
