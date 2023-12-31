/*
 * Copyright (c) 2015-2021 TRUSTONIC LIMITED
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
#ifndef HalAdaptationLayer_H
#define HalAdaptationLayer_H

#include <memory>
#include <stdlib.h>
#include <hardware/hardware.h>
#include <hardware/gatekeeper.h>

#undef  LOG_TAG
#define LOG_TAG "TlcTeeGatekeeper"

class TeeSession;

struct HalAdaptationLayer
{
    // device_ must always be first member of a struct
    gatekeeper_device_t          _device;
    std::unique_ptr<TeeSession>  _session;

/* -----------------------------------------------------------------------------
 * @brief   Method implements HAL interface of gatekeeper.
 *          Enrolls desired_password, which should be derived from a user selected pin or password,
 *          with the authentication factor private key used only for enrolling authentication
 *          factor data.
 *          If there was already a password enrolled, it should be provided in
 *          current_password_handle, along with the current password in current_password
 *          that should validate against current_password_handle.
 *
 * @param   dev: pointer to gatekeeper_device acquired via calls to gatekeeper_open
 * @param   uid: the Android user identifier
 *
 * @param   current_password_handle: the currently enrolled password handle the user
 *          wants to replace. May be null if there's no currently enrolled password.
 * @param   current_password_handle_length: the length in bytes of the buffer pointed
 *          at by current_password_handle. Must be 0 if current_password_handle is NULL.
 *
 * @param   current_password: the user's current password in plain text. If presented,
 *          it MUST verify against current_password_handle.
 * @param   current_password_length: the size in bytes of the buffer pointed at by
 *          current_password. Must be 0 if the current_password is NULL.
 *
 * @param   desired_password: the new password the user wishes to enroll in plain-text
 *          Cannot be NULL.
 * @param   desired_password_length: the length in bytes of the buffer pointed at by
 *          desired_password.
 *
 * @param   enrolled_password_handle: on success, a buffer will be allocated with the
 *          new password handle referencing the password provided in desired_password.
 *          This buffer can be used on subsequent calls to enroll or verify.
 *          The caller is responsible for deallocating this buffer via a call to delete[].
 *          On error, enrolled_password_handle is not allocated.

 * @param   enrolled_password_handle_length: pointer to the length in bytes of the buffer allocated
 *          by this function and pointed to by *enrolled_password_handle_length.
 *
 * @return  Returned values are members of gatekeeper::gatekeeper_error_t enum.
 *
 *          ERROR_NONE: Success
 *          An error code < 0 on failure:
 *            -ERROR_INVALID: unexpected value of input parameters or verification
 *                            of current_password has failed.
 *            -ERROR_UNKNOWN: any unexpected behaviour (see logs)
 *            -ERROR_RETRY:   unused currently
 *
 *          Any positive value T > 0 if the call should not be re-attempted
 *          until T milliseconds have elapsed.
-------------------------------------------------------------------------------- */
static int enroll( const struct gatekeeper_device *dev, uint32_t uid,
            const uint8_t *current_password_handle, uint32_t current_password_handle_length,
            const uint8_t *current_password, uint32_t current_password_length,
            const uint8_t *desired_password, uint32_t desired_password_length,
            uint8_t **enrolled_password_handle, uint32_t *enrolled_password_handle_length);

/**
 * @brief   Verifies provided_password matches enrolled_password_handle.
 *
 *          Implementations of this module may retain the result of this call
 *          to attest to the recency of authentication.
 *
 *          On success, writes the address of a verification token to auth_token,
 *          usable to attest password verification to other trusted services. Clients
 *          may pass NULL for this value.
 *
 * @param   dev: pointer to gatekeeper_device acquired via calls to gatekeeper_open
 * @param   uid: the Android user identifier
 *
 * @param   challenge: An optional challenge to authenticate against, or 0. Used when a separate
 *          authenticator requests password verification, or for transactional
 *          password authentication.
 *
 * @param   enrolled_password_handle: the currently enrolled password handle that the
 *          user wishes to verify against.
 *
 * @param   enrolled_password_handle_length: the length in bytes of the buffer pointed
 *          to by enrolled_password_handle
 *
 * @param   provided_password: the plaintext password to be verified against the
 *          enrolled_password_handle
 *
 * @param   provided_password_length: the length in bytes of the buffer pointed to by
 *          provided_password
 *
 * @param   auth_token: on success, a buffer containing the authentication token
 *          resulting from this verification is assigned to *auth_token. The caller
 *          is responsible for deallocating this memory via a call to delete[]
 *
 * @param   auth_token_length: on success, the length in bytes of the authentication
 *          token assigned to *auth_token will be assigned to *auth_token_length
 *
 * @param   request_reenroll: a request to the upper layers to re-enroll the verified
 *          password due to a version change. Not set if verification fails.
 *
 * Returns:
 * - 0 on success
 * - An error code < 0 on failure, or
 * - A timeout value T > 0 if the call should not be re-attempted until T milliseconds
 *   have elapsed.
 * On error, auth token will not be allocated
 */

static int verify(const struct gatekeeper_device *dev, uint32_t uid, uint64_t challenge,
        const uint8_t *enrolled_password_handle, uint32_t enrolled_password_handle_length,
        const uint8_t *provided_password, uint32_t provided_password_length,
        uint8_t **auth_token, uint32_t *auth_token_length, bool *request_reenroll);

/* -----------------------------------------------------------------------------
 * @brief   Deletes the enrolled_password_handle associated wth the uid. Once deleted
 *          the user cannot be verified anymore.
 *          This function is optional and should be set to NULL if it is not implemented.
 *
 * @param   dev: pointer to gatekeeper_device acquired via calls to gatekeeper_open
 * @param   uid: the Android user identifier
 *
 * @returns:
 *          ERROR_NONE: Success
 *          An error code < 0 on failure
-------------------------------------------------------------------------------- */
static int delete_user(const struct gatekeeper_device *dev, uint32_t uid);

/* -----------------------------------------------------------------------------
 * @brief   Deletes all the enrolled_password_handles for all uid's. Once called,
 *          no users will be enrolled on the device.
 *          This function is optional and should be set to NULL if it is not implemented.
 *
 * @param   dev: pointer to gatekeeper_device acquired via calls to gatekeeper_open
 *
 * @returns
 *          - ERROR_NONE: on success
 *          - An error code < 0 on failure
 --------------------------------------------------------------------------------*/
    static int delete_all_users(const struct gatekeeper_device* dev);

    explicit HalAdaptationLayer(hw_module_t* module);

    bool Initialize();
    void Terminate();

    ~HalAdaptationLayer();
};

/*
 * Specialization used by std::unique_ptr. Makes sure gatekeeper device
 * is closed before deleting objects. Resolved by ADL
 */
namespace std {
    template<>
    class default_delete<HalAdaptationLayer>
    {
    public:
        void operator()(HalAdaptationLayer* gatekeeper) const
        {
            (void)gatekeeper->Terminate();
            delete gatekeeper;
        }
    };
} // namespace std

#endif /* HalAdaptationLayer_H */
