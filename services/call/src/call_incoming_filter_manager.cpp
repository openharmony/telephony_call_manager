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
#include "call_incoming_filter_manager.h"

#include "securec.h"

#include "call_manager_errors.h"
#include "telephony_log_wrapper.h"
#include "call_manager_inner_type.h"
#include "call_control_manager.h"
#include "call_data_base_helper.h"
#include "cellular_call_connection.h"

namespace OHOS {
namespace Telephony {
CallIncomingFilterManager::CallIncomingFilterManager() : isFirstIncoming(true) {}

CallIncomingFilterManager::~CallIncomingFilterManager() {}

int32_t CallIncomingFilterManager::PackCellularCallInfo(CellularCallInfo &callInfo, const CallDetailInfo &info)
{
    callInfo.callType = info.callType;
    callInfo.index = info.index;
    callInfo.slotId = info.accountId;
    callInfo.accountId = info.accountId;
    if (memset_s(callInfo.phoneNum, kMaxNumberLen, 0, kMaxNumberLen) != EOK) {
        TELEPHONY_LOGW("memset_s failed!");
        return TELEPHONY_ERR_MEMSET_FAIL;
    }
    if (memcpy_s(callInfo.phoneNum, kMaxNumberLen, info.phoneNum, kMaxNumberLen) != EOK) {
        TELEPHONY_LOGW("memcpy_s failed!");
        return TELEPHONY_ERR_MEMCPY_FAIL;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallIncomingFilterManager::DoIncomingFilter(const CallDetailInfo &info)
{
    bool isEcc = false;
    if (!DelayedSingleton<CallControlManager>::GetInstance()->IsIncomingEnable(std::string(info.phoneNum))) {
        TELEPHONY_LOGE("MDM policy is disabled!");
        return TELEPHONY_ERR_POLICY_DISABLED;
    }
    DelayedSingleton<CellularCallConnection>::GetInstance()->IsEmergencyPhoneNumber(
        info.phoneNum, info.accountId, isEcc);
    if (isEcc) {
        TELEPHONY_LOGI("incoming phoneNumber is ecc.");
        return TELEPHONY_SUCCESS;
    }
    return TELEPHONY_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS