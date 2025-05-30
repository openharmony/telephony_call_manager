/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TELEPHONY_CALL_FFI_H
#define TELEPHONY_CALL_FFI_H

#include "ffi_remote_data.h"
#include "telephony_call_utils.h"
#include "telephony_call_impl.h"

namespace OHOS {
namespace Telephony {
extern "C" {
    FFI_EXPORT bool FfiOHOSTelephonyCallIsRinging(int32_t *errCode);

    FFI_EXPORT char* FfiOHOSTelephonyCallFormatPhoneNumberToE164(char *cPhoneNumber, char *cCountryCode,
        int32_t *errCode);

    FFI_EXPORT void FfiOHOSTelephonyCallInit();

    FFI_EXPORT int32_t FfiOHOSTelephonyCallDialCall(char *phoneNumber, DialCallOptions options);

    FFI_EXPORT bool FfiOHOSTelephonyCallHasVoiceCapability();

    FFI_EXPORT char* FfiOHOSTelephonyCallFormatPhoneNumber(char *phoneNumber, char *countryCode, int32_t *errCode);

    FFI_EXPORT int32_t FfiOHOSTelephonyCallGetCallState();

    FFI_EXPORT bool FfiOHOSTelephonyCallHasCall();

    FFI_EXPORT bool FfiOHOSTelephonyCallIsEmergencyPhoneNumber(char *phoneNumber, int32_t slotId, int32_t* errCode);

    FFI_EXPORT int32_t FfiOHOSTelephonyCallMakeCall(char* number);
}
}
}

#endif