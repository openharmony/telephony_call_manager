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

#include <string_ex.h>

#include "app_mgr_interface.h"
#include "app_state_observer.h"
#include "call_manager_errors.h"
#include "iservice_registry.h"
#include "system_ability.h"
#include "system_ability_definition.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CallAbilityReportProxy::CallAbilityReportProxy()
{
    callbackPtrList_.clear();
}

CallAbilityReportProxy::~CallAbilityReportProxy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    while (it != callbackPtrList_.end()) {
        if ((*it)) {
            (*it).clear();
            (*it) = nullptr;
        }
        callbackPtrList_.erase(it++);
    }
}

int32_t CallAbilityReportProxy::RegisterCallBack(
    sptr<ICallAbilityCallback> callAbilityCallbackPtr, const std::string &bundleName)
{
    if (callAbilityCallbackPtr == nullptr) {
        TELEPHONY_LOGE("callAbilityCallbackPtr is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callAbilityCallbackPtr->SetBundleName(bundleName);
    std::lock_guard<std::mutex> lock(mutex_);
    callbackPtrList_.emplace_back(callAbilityCallbackPtr);
    TELEPHONY_LOGI("%{public}s successfully registered the callback!", bundleName.c_str());
    if (appStateObserver == nullptr) {
        appStateObserver = new (std::nothrow) ApplicationStateObserver();
        if (appStateObserver == nullptr) {
            TELEPHONY_LOGE("Failed to Create AppStateObserver Instance");
            return TELEPHONY_SUCCESS;
        }
        sptr<ISystemAbilityManager> samgrClient = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgrClient == nullptr) {
            TELEPHONY_LOGE("Failed to get samgrClient");
            appStateObserver = nullptr;
            return TELEPHONY_SUCCESS;
        }
        appMgrProxy = iface_cast<AppExecFwk::IAppMgr>(samgrClient->GetSystemAbility(APP_MGR_SERVICE_ID));
        if (appMgrProxy == nullptr) {
            TELEPHONY_LOGE("Failed to get appMgrProxy");
            appStateObserver = nullptr;
            samgrClient = nullptr;
            return TELEPHONY_SUCCESS;
        }
        appMgrProxy->RegisterApplicationStateObserver(appStateObserver);
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallAbilityReportProxy::UnRegisterCallBack(const std::string &bundleName)
{
    if (callbackPtrList_.empty()) {
        TELEPHONY_LOGE("callbackPtrList_ is null! %{public}s UnRegisterCallBack failed", bundleName.c_str());
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)->GetBundleName() == bundleName) {
            callbackPtrList_.erase(it);
            TELEPHONY_LOGI("%{public}s UnRegisterCallBack success", bundleName.c_str());
            break;
        }
    }
    if (callbackPtrList_.empty()) {
        TELEPHONY_LOGE("callbackPtrList_ is null!");
        if (appMgrProxy != nullptr && appStateObserver != nullptr) {
            int ret = appMgrProxy->UnregisterApplicationStateObserver(appStateObserver);
            TELEPHONY_LOGI("UnregisterApplicationStateObserver result = %{public}d", ret);
            appMgrProxy = nullptr;
            appStateObserver = nullptr;
        }
    }
    return TELEPHONY_SUCCESS;
}

void CallAbilityReportProxy::CallStateUpdated(
    sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState)
{
    if (callObjectPtr == nullptr) {
        TELEPHONY_LOGE("callObjectPtr is nullptr!");
        return;
    }
    CallAttributeInfo info;
    callObjectPtr->GetCallAttributeInfo(info);
    size_t accountLen = strlen(info.accountNumber);
    if (accountLen > static_cast<size_t>(kMaxNumberLen)) {
        accountLen = kMaxNumberLen;
    }
    for (size_t i = 0; i < accountLen; i++) {
        if (info.accountNumber[i] == ',' || info.accountNumber[i] == ';') {
            info.accountNumber[i] = '\0';
            break;
        }
    }
    if (nextState == TelCallState::CALL_STATUS_ANSWERED) {
        TELEPHONY_LOGI("report answered state");
        info.callState = TelCallState::CALL_STATUS_ANSWERED;
    }
    ReportCallStateInfo(info);
}

void CallAbilityReportProxy::CallEventUpdated(CallEventInfo &info)
{
    ReportCallEvent(info);
}

void CallAbilityReportProxy::CallDestroyed(const DisconnectedDetails &details)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnCallDisconnectedCause(details);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("OnCallDisconnectedCause failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("report call disconnected cause[%{public}d] success", details.reason);
}

int32_t CallAbilityReportProxy::ReportCallStateInfo(const CallAttributeInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::string bundleName = "";
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            bundleName = (*it)->GetBundleName();
            ret = (*it)->OnCallDetailsChange(info);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW(
                    "OnCallDetailsChange failed, errcode:%{public}d, bundleName:%{public}s", ret, bundleName.c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("report call state info success, callId[%{public}d] state[%{public}d] conferenceState[%{public}d] "
                   "videoState[%{public}d]",
        info.callId, info.callState, info.conferenceState, info.videoState);
    return ret;
}

int32_t CallAbilityReportProxy::ReportCallStateInfo(const CallAttributeInfo &info, std::string bundleName)
{
    int32_t ret = TELEPHONY_ERROR;
    for (auto callback : callbackPtrList_) {
        if (callback->GetBundleName() == bundleName) {
            ret = callback->OnCallDetailsChange(info);
            break;
        }
    }
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE(
            "OnCallDetailsChange failed, errcode:%{public}d, bundleName:%{public}s", ret, bundleName.c_str());
    } else {
        TELEPHONY_LOGI("callId[%{public}d] state[%{public}d] conferenceState[%{public}d] "
                       "videoState[%{public}d], report bundleName %{public}s success",
            info.callId, info.callState, info.conferenceState, info.videoState, bundleName.c_str());
    }
    return ret;
}

int32_t CallAbilityReportProxy::ReportCallEvent(const CallEventInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    TELEPHONY_LOGI("report call event, eventId:%{public}d", info.eventId);
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnCallEventChange(info);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("OnCallEventChange failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("report call event[%{public}d] info success", info.eventId);
    return ret;
}

int32_t CallAbilityReportProxy::ReportAsyncResults(
    const CallResultReportId reportId, AppExecFwk::PacMap &resultInfo)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportAsyncResults(reportId, resultInfo);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportAsyncResults failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportAsyncResults success, reportId:%{public}d", reportId);
    return ret;
}

int32_t CallAbilityReportProxy::ReportMmiCodeResult(const MmiCodeInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportMmiCodeResult(info);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportMmiCodeResult failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportMmiCodeResult success");
    return ret;
}

int32_t CallAbilityReportProxy::OttCallRequest(OttCallRequestId requestId, AppExecFwk::PacMap &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        std::string name = (*it)->GetBundleName();
        if (name == "com.ohos.callservice") {
            ret = (*it)->OnOttCallRequest(requestId, info);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW(
                    "OttCallRequest failed, errcode:%{public}d, bundleName:%{public}s", ret, name.c_str());
                break;
            }
        }
    }
    TELEPHONY_LOGI("OttCallRequest success, requestId:%{public}d", requestId);
    return ret;
}

int32_t CallAbilityReportProxy::ReportAudioDeviceChange(const AudioDeviceInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportAudioDeviceChange(info);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportAudioDeviceChange failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportAudioDeviceChange success");
    return ret;
}

int32_t CallAbilityReportProxy::ReportPostDialDelay(const std::string &str)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportPostDialDelay(str);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportPostDialDelay failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportPostDialDelay success");
    return ret;
}

int32_t CallAbilityReportProxy::ReportImsCallModeChange(const CallMediaModeInfo &imsCallModeInfo)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportImsCallModeChange(imsCallModeInfo);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportImsCallModeReceive failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportImsCallModeReceive success");
    return ret;
}

int32_t CallAbilityReportProxy::ReportCallSessionEventChange(
    const CallSessionEvent &callSessionEventOptions)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportCallSessionEventChange(callSessionEventOptions);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportCallSessionEventChange failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportCallSessionEventChange success");
    return ret;
}

int32_t CallAbilityReportProxy::ReportPeerDimensionsChange(const PeerDimensionsDetail &peerDimensionsDetail)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportPeerDimensionsChange(peerDimensionsDetail);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportPeerDimensionsChange failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportPeerDimensionsChange success");
    return ret;
}

int32_t CallAbilityReportProxy::ReportCallDataUsageChange(const int64_t dataUsage)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportCallDataUsageChange(dataUsage);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportCallDataUsageChange failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportCallDataUsageChange success");
    return ret;
}

int32_t CallAbilityReportProxy::ReportCameraCapabilities(const CameraCapabilities &cameraCapabilities)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<sptr<ICallAbilityCallback>>::iterator it = callbackPtrList_.begin();
    for (; it != callbackPtrList_.end(); ++it) {
        if ((*it)) {
            ret = (*it)->OnReportCameraCapabilities(cameraCapabilities);
            if (ret != TELEPHONY_SUCCESS) {
                TELEPHONY_LOGW("ReportCameraCapabilities failed, errcode:%{public}d, bundleName:%{public}s", ret,
                    ((*it)->GetBundleName()).c_str());
                continue;
            }
        }
    }
    TELEPHONY_LOGI("ReportCameraCapabilities success");
    return ret;
}
} // namespace Telephony
} // namespace OHOS
