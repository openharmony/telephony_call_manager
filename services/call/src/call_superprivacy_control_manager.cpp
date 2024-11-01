/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#include "call_ability_report_proxy.h"
#include "call_superprivacy_control_manager.h"
#include "call_number_utils.h"
#include "call_control_manager.h"
#include "syspara/parameters.h"
#include "super_privacy_manager_client.h"
#include "display_manager.h"
#include "display_info.h"
#include "call_manager_hisysevent.h"

namespace OHOS {
namespace Telephony {
const std::string SUPER_PRIVACY_MODE_PARAM_KEY = "persist.super_privacy.mode";
const char SUPER_PRIVACY_MODE_PARAM_KEYS[] = "persist.super_privacy.mode";
const char SUPER_PRIVACY_MODE_PARAM_OPEN[] = "2";
const char SUPER_PRIVACY_MODE_PARAM_CLOSE[] = "0";
const int32_t SOURCE_CALL = 2;

void CallSuperPrivacyControlManager::RegisterSuperPrivacyMode()
{
    int32_t ret = WatchParameter(SUPER_PRIVACY_MODE_PARAM_KEYS, ParamChangeCallback, nullptr);
    TELEPHONY_LOGE("RegisterSuperPrivacyMode ret:%{public}d", ret);
}

void CallSuperPrivacyControlManager::ParamChangeCallback(const char *key, const char *value, void *context)
{
    SuperPrivacyModeChangeEvent();
    if (key == nullptr ||value == nullptr) {
        return;
    }
    if (strcmp(key, SUPER_PRIVACY_MODE_PARAM_KEYS)) {
        return;
    }
    std::string keyStr(key);
    std::string valueStr(value);
    TELEPHONY_LOGE("ParamChangeCallback keyStr:%{public}s", keyStr.c_str());
    TELEPHONY_LOGE("ParamChangeCallback valueStr:%{public}s", valueStr.c_str());
    bool isSuperPrivacyModeOpen = strcmp(value, SUPER_PRIVACY_MODE_PARAM_OPEN) == 0 ? true : false;
    bool isSuperPrivacyModeClose = strcmp(value, SUPER_PRIVACY_MODE_PARAM_CLOSE) == 0 ? true : false;
    if (isSuperPrivacyModeOpen) {
        DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->SetIsChangeSuperPrivacyMode(false);
        DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->CloseAllCall();
    } else if (isSuperPrivacyModeClose) {
        bool isChangeSuperPrivacyMode = DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->
            GetIsChangeSuperPrivacyMode();
        if (!isChangeSuperPrivacyMode) {
            DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->SetIsChangeSuperPrivacyMode(false);
        }
    }
}

void CallSuperPrivacyControlManager::SuperPrivacyModeChangeEvent()
{
    CallEventInfo eventInfo;
    (void)memset_s(&eventInfo, sizeof(CallEventInfo), 0, sizeof(CallEventInfo));
    bool isSuperPrivacyMode = DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->
            GetCurrentIsSuperPrivacyMode();
    TELEPHONY_LOGI("SuperPrivacyMode:%{public}d", isSuperPrivacyMode);
    if (isSuperPrivacyMode) {
        eventInfo.eventId = CallAbilityEventId::EVENT_IS_SUPER_PRIVACY_MODE_ON;
    } else {
        eventInfo.eventId = CallAbilityEventId::EVENT_IS_SUPER_PRIVACY_MODE_OFF;
    }
    DelayedSingleton<CallAbilityReportProxy>::GetInstance()->CallEventUpdated(eventInfo);
}

void CallSuperPrivacyControlManager::CloseAllCall()
{
    std::vector<CallAttributeInfo> infos = CallObjectManager::GetAllCallInfoList();
    for (auto &info : infos) {
        if (!info.isEcc && !info.isEccContact) {
            TELEPHONY_LOGE("OnSuperPrivacyModeChanged callState:%{public}d", info.callState);
            if (info.callState == TelCallState::CALL_STATUS_INCOMING ||
                info.callState == TelCallState::CALL_STATUS_WAITING) {
                DelayedSingleton<CallControlManager>::GetInstance()->RejectCall(info.callId, false,
                u"superPrivacyModeOn");
            } else {
                DelayedSingleton<CallControlManager>::GetInstance()->HangUpCall(info.callId);
            }
        }
    }
}

bool CallSuperPrivacyControlManager::GetIsChangeSuperPrivacyMode()
{
    return isChangeSuperPrivacyMode;
}

void CallSuperPrivacyControlManager::SetIsChangeSuperPrivacyMode(bool isChangeSuperPrivacy)
{
    isChangeSuperPrivacyMode = isChangeSuperPrivacy;
}

void CallSuperPrivacyControlManager::SetOldSuperPrivacyMode()
{
    int32_t privpacyMode = system::GetIntParameter(SUPER_PRIVACY_MODE_PARAM_KEY.c_str(), -1);
    TELEPHONY_LOGE("SetOldSuperPrivacyMode privpacyMode:%{public}d", privpacyMode);
    oldSuperPrivacyMode = privpacyMode;
}

int32_t CallSuperPrivacyControlManager::GetOldSuperPrivacyMode()
{
    return oldSuperPrivacyMode;
}

int32_t CallSuperPrivacyControlManager::CloseSuperPrivacyMode()
{
    int32_t privacy = SuperPrivacyManagerClient::GetInstance().
        SetSuperPrivacyMode(static_cast<int32_t>(CallSuperPrivacyModeType::OFF), SOURCE_CALL);
    TELEPHONY_LOGE("CloseSuperPrivacyMode privacy:%{public}d", privacy);
    return privacy;
}

void CallSuperPrivacyControlManager::CloseCallSuperPrivacyMode(std::u16string &phoneNumber, int32_t &accountId,
    int32_t &videoState, int32_t &dialType, int32_t &dialScene, int32_t &callType)
{
    int32_t privacy = CloseSuperPrivacyMode();
    TELEPHONY_LOGE("CloseCallSuperPrivacyMode privacy:%{public}d", privacy);
    if (privacy == SUPER_PRIVACY_MODE_REQUEST_SUCCESS) {
        AppExecFwk::PacMap dialInfo;
        dialInfo.PutIntValue("accountId", accountId);
        dialInfo.PutIntValue("videoState", videoState);
        dialInfo.PutIntValue("dialType", dialType);
        dialInfo.PutIntValue("dialScene", dialScene);
        dialInfo.PutIntValue("callType", callType);
        int32_t ret = DelayedSingleton<CallControlManager>::GetInstance()->DialCall(phoneNumber, dialInfo);
        if (ret != TELEPHONY_SUCCESS) {
            RestoreSuperPrivacyMode();
        }
    }
    CallManagerHisysevent::HiWriteBehaviorEventPhoneUE(
        CALL_DIAL_CLOSE_SUPER_PRIVACY, PNAMEID_KEY, KEY_CALL_MANAGER, PVERSIONID_KEY, "");
}

void CallSuperPrivacyControlManager::CloseAnswerSuperPrivacyMode(int32_t callId, int32_t videoState)
{
    int32_t privacy = CloseSuperPrivacyMode();
    TELEPHONY_LOGE("CloseAnswerSuperPrivacyMode privacy:%{public}d", privacy);
    if (privacy == SUPER_PRIVACY_MODE_REQUEST_SUCCESS) {
        DelayedSingleton<CallControlManager>::GetInstance()->AnswerCall(callId, videoState);
        SuperPrivacyModeChangeEvent();
    }
}

void CallSuperPrivacyControlManager::RestoreSuperPrivacyMode()
{
    if (!GetIsChangeSuperPrivacyMode()) {
        return;
    }
    int32_t privpacyMode = system::GetIntParameter(SUPER_PRIVACY_MODE_PARAM_KEY.c_str(), -1);
    int32_t oldPrivpacy = GetOldSuperPrivacyMode();
    TELEPHONY_LOGE("RestoreSuperPrivacyMode oldPrivpacy:%{public}d", oldPrivpacy);
    if (privpacyMode != oldPrivpacy) {
        SetIsChangeSuperPrivacyMode(false);
        if (oldPrivpacy == static_cast<int32_t>(CallSuperPrivacyModeType::ALWAYS_ON)
            || oldPrivpacy == static_cast<int32_t>(CallSuperPrivacyModeType::ON_WHEN_FOLDED)) {
            int32_t privacy = SuperPrivacyManagerClient::GetInstance().SetSuperPrivacyMode(oldPrivpacy, SOURCE_CALL);
            TELEPHONY_LOGE("RestoreSuperPrivacyMode ret privacy:%{public}d", privacy);
        }
    }
}

bool CallSuperPrivacyControlManager::GetCurrentIsSuperPrivacyMode()
{
    int32_t privpacyMode = system::GetIntParameter(SUPER_PRIVACY_MODE_PARAM_KEY.c_str(), -1);
    TELEPHONY_LOGE("GetCurrentIsSuperPrivacyMode privpacyMode:%{public}d", privpacyMode);
    if (privpacyMode == static_cast<int32_t>(CallSuperPrivacyModeType::ALWAYS_ON)) {
        return true;
    }
    Rosen::FoldStatus foldStatus = Rosen::DisplayManager::GetInstance().GetFoldStatus();
    if (privpacyMode == static_cast<int32_t>(CallSuperPrivacyModeType::ON_WHEN_FOLDED)
        && foldStatus == Rosen::FoldStatus::FOLDED) {
        TELEPHONY_LOGI("GetCurrentIsSuperPrivacyMode ON_WHEN_FOLDED");
        return true;
    }
    return false;
}
} // namespace Telephony
} // namespace OHOS