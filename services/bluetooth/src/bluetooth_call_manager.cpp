/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "bluetooth_call_manager.h"

#include "call_ability_report_proxy.h"
#include "call_manager_errors.h"
#include "telephony_log_wrapper.h"

#include "audio_control_manager.h"
#include "call_control_manager.h"
#include "bluetooth_connection.h"

namespace OHOS {
namespace Telephony {
BluetoothCallManager::BluetoothCallManager() : btConnection_(DelayedSingleton<BluetoothConnection>::GetInstance()) {}

BluetoothCallManager::~BluetoothCallManager() {}

int32_t BluetoothCallManager::SendBtCallState(
    int32_t numActive, int32_t numHeld, int32_t callState, const std::string &number)
{
    DelayedSingleton<CallAbilityReportProxy>::GetInstance()->ReportPhoneStateChange(numActive, numHeld, callState,
        number);
    if (btConnection_ == nullptr) {
        TELEPHONY_LOGE("bluetooth connection nullptr");
        return false;
    }
    return btConnection_->SendBtCallState(numActive, numHeld, callState, number);
}

int32_t BluetoothCallManager::SendCallDetailsChange(int32_t callId, int32_t callState)
{
    if (btConnection_ == nullptr) {
        TELEPHONY_LOGE("bluetooth connection nullptr");
        return false;
    }
    return btConnection_->SendCallDetailsChange(callId, callState);
}

bool BluetoothCallManager::IsBtAvailble()
{
    if (btConnection_ == nullptr) {
        TELEPHONY_LOGE("bluetooth connection nullptr");
        return false;
    }
    bool isBtAvailble = false;

#ifdef ABILITY_BLUETOOTH_SUPPORT
    isBtAvailble = btConnection_->IsBtAvailble();
#endif
    return isBtAvailble;
}
} // namespace Telephony
} // namespace OHOS