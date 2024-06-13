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

#ifndef CALL_SUPERPRIVACY_CONTROL_MANAGER_H
#define CALL_SUPERPRIVACY_CONTROL_MANAGER_H

#include "singleton.h"
#ifdef SUPPORT_SUPER_PRIVACY_SERVICE
#include "model/super_privacy_mode_listener.h"
#endif


/**
 * @ClassName: CallSuperPrivacyControlManager
 * @Description: check super privacy mode in CallSuperPrivacyControlManager
 */
namespace OHOS {
namespace Telephony {
#ifdef SUPPORT_SUPER_PRIVACY_SERVICE
using namespace AppSecurityPrivacy::SecurityPrivacyServer::SuperPrivacy;
class CallSuperPrivacyListener : public SuperPrivacyModeListener {
public:
    CallSuperPrivacyListener() = default;
    void OnSuperPrivacyModeChanged(const int32_t &superPrivacyMode) override;
};
#endif
class CallSuperPrivacyControlManager {
public:
    static const int SUPER_PRIVACY_MODE_REQUEST_SUCCESS = 0;
    bool GetIsChangeSuperPrivacyMode();
    void SetIsChangeSuperPrivacyMode(bool isChangeSuperPrivacyMode);
    int32_t GetOldSuperPrivacyMode();
    void SetOldSuperPrivacyMode();
    int32_t CloseSuperPrivacyMode();
    void CloseCallSuperPrivacyMode(std::u16string &number, int32_t &accountId,
	int32_t &videoState, int32_t &dialType, int32_t &dialScene, int32_t &callType);
    void CloseAnswerSuperPrivacyMode(int32_t callId, int32_t videoState);
    void RestoreSuperPrivacyMode();
    bool GetCurrentIsSuperPrivacyMode();
    void RegisterSuperPrivacyMode();
    void UnRegisterSuperPrivacyMode();
    void CloseAllCall();
    
private:
    bool isChangeSuperPrivacyMode = false;
    int32_t oldSuperPrivacyMode = -1;
#ifdef SUPPORT_SUPER_PRIVACY_SERVICE
    std::shared_ptr<SuperPrivacyModeListener> modeChangeListener_ = nullptr;
#endif
};
} // namespace Telephony
} // namespace OHOS

#endif // CALL_POLICY_H