/*
 * Copyright (c) 2018 TRUSTONIC LIMITED
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

#ifndef TEE_SERVICE_GP_SHARED_MEMORY_IN_H
#define TEE_SERVICE_GP_SHARED_MEMORY_IN_H

#include <binder/Parcel.h>

namespace vendor {

namespace trustonic {

namespace teeservice {

struct TeeServiceGpSharedMemoryIn : public ::android::Parcelable {
    uint64_t context = 0;
    uint64_t reference = 0;
    int buffer = -1;
    uint32_t size = 0;
    uint32_t flags = 0;

    virtual ::android::status_t writeToParcel(::android::Parcel* parcel) const {
        parcel->writeUint64(context);
        parcel->writeUint64(reference);
        if (buffer < 0) {
            parcel->writeBool(false);
        } else {
            parcel->writeBool(true);
            parcel->writeFileDescriptor(buffer);
        }

        parcel->writeUint32(size);
        parcel->writeUint32(flags);
        return ::android::OK;
    }

    virtual ::android::status_t readFromParcel(const ::android::Parcel* parcel) {
        parcel->readUint64(&context);
        parcel->readUint64(&reference);
        if (parcel->readBool()) {
            buffer = ::dup(parcel->readFileDescriptor());
        }

        parcel->readUint32(&size);
        parcel->readUint32(&flags);
        return ::android::OK;
    }
};

}

}

}

#endif // TEE_SERVICE_GP_SHARED_MEMORY_IN_H
