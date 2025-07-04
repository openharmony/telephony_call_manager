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

#include "napi_call_ability_callback.h"

#include <securec.h>
#include <ctime>

#include "call_manager_errors.h"
#include "napi_call_manager_utils.h"
#include "napi_util.h"
#include "pixel_map.h"
#include "pixel_map_napi.h"
#include "telephony_log_wrapper.h"
#include "napi_common_want.h"
#include "want_params_wrapper.h"

namespace OHOS {
namespace Telephony {
EventCallback NapiCallAbilityCallback::audioDeviceCallback_;
std::mutex NapiCallAbilityCallback::audioDeviceCallbackMutex_;
NapiCallAbilityCallback::NapiCallAbilityCallback()
{
    (void)memset_s(&stateCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&meeTimeStateCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&eventCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&ottRequestCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&setRestrictionCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&startRttCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&stopRttCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&callDisconnectCauseCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&mmiCodeCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&postDialDelayCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&imsCallModeCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&peerDimensionsCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&callDataUsageCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&cameraCapabilitiesCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    (void)memset_s(&callSessionEventCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    memberFuncMap_[CallResultReportId::GET_CALL_WAITING_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportGetWaitingInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::SET_CALL_WAITING_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportSetWaitingInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::GET_CALL_RESTRICTION_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportGetRestrictionInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::SET_CALL_RESTRICTION_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportSetRestrictionInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::SET_CALL_RESTRICTION_PWD_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportSetRestrictionPassword(resultInfo); };
    memberFuncMap_[CallResultReportId::GET_CALL_TRANSFER_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportGetTransferInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::SET_CALL_TRANSFER_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportSetTransferInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::START_RTT_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportStartRttInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::STOP_RTT_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportStopRttInfo(resultInfo); };
    memberFuncMap_[CallResultReportId::CLOSE_UNFINISHED_USSD_REPORT_ID] =
        [this](AppExecFwk::PacMap &resultInfo) { return ReportCloseUnFinishedUssdInfo(resultInfo); };
    UnRegisterGetWaitingCallback();
    UnRegisterSetWaitingCallback();
    UnRegisterGetRestrictionCallback();
    UnRegisterSetRestrictionPasswordCallback();
    UnRegisterGetTransferCallback();
    UnRegisterSetTransferCallback();
}

NapiCallAbilityCallback::~NapiCallAbilityCallback()
{
    TELEPHONY_LOGI("~NapiCallAbilityCallback");
}

void NapiCallAbilityCallback::RegisterCallStateCallback(EventCallback stateCallback)
{
    stateCallback_ = stateCallback;
}

void NapiCallAbilityCallback::UnRegisterCallStateCallback()
{
    if (stateCallback_.callbackRef) {
        napi_delete_reference(stateCallback_.env, stateCallback_.callbackRef);
        napi_delete_reference(stateCallback_.env, stateCallback_.thisVar);
        (void)memset_s(&stateCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterMmiCodeCallback(EventCallback eventCallback)
{
    mmiCodeCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterMmiCodeCallback()
{
    if (mmiCodeCallback_.callbackRef) {
        napi_delete_reference(mmiCodeCallback_.env, mmiCodeCallback_.callbackRef);
        napi_delete_reference(mmiCodeCallback_.env, mmiCodeCallback_.thisVar);
        (void)memset_s(&mmiCodeCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterAudioDeviceCallback(EventCallback eventCallback)
{
    std::lock_guard<std::mutex> lock(audioDeviceCallbackMutex_);
    audioDeviceCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterAudioDeviceCallback()
{
    std::lock_guard<std::mutex> lock(audioDeviceCallbackMutex_);
    if (audioDeviceCallback_.callbackRef) {
        napi_delete_reference(audioDeviceCallback_.env, audioDeviceCallback_.callbackRef);
        napi_delete_reference(audioDeviceCallback_.env, audioDeviceCallback_.thisVar);
        (void)memset_s(&audioDeviceCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
        TELEPHONY_LOGI("unregister audio device callback end.");
    }
}

void NapiCallAbilityCallback::RegisterPostDialDelay(EventCallback eventCallback)
{
    postDialDelayCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterPostDialDelayCallback()
{
    if (postDialDelayCallback_.callbackRef) {
        napi_delete_reference(postDialDelayCallback_.env, postDialDelayCallback_.callbackRef);
        napi_delete_reference(postDialDelayCallback_.env, postDialDelayCallback_.thisVar);
        (void)memset_s(&postDialDelayCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterCallEventCallback(EventCallback eventCallback)
{
    eventCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterCallEventCallback()
{
    if (eventCallback_.callbackRef) {
        napi_delete_reference(eventCallback_.env, eventCallback_.callbackRef);
        napi_delete_reference(eventCallback_.env, eventCallback_.thisVar);
        (void)memset_s(&eventCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterImsCallModeChangeCallback(EventCallback eventCallback)
{
    imsCallModeCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterImsCallModeChangeCallback()
{
    if (eventCallback_.callbackRef) {
        napi_delete_reference(imsCallModeCallback_.env, imsCallModeCallback_.callbackRef);
        napi_delete_reference(imsCallModeCallback_.env, imsCallModeCallback_.thisVar);
        (void)memset_s(&imsCallModeCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterPeerDimensionsChangeCallback(EventCallback eventCallback)
{
    peerDimensionsCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterPeerDimensionsChangeCallback()
{
    if (eventCallback_.callbackRef) {
        napi_delete_reference(peerDimensionsCallback_.env, peerDimensionsCallback_.callbackRef);
        napi_delete_reference(peerDimensionsCallback_.env, peerDimensionsCallback_.thisVar);
        (void)memset_s(&peerDimensionsCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterCallDataUsageChangeCallback(EventCallback eventCallback)
{
    callDataUsageCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterCallDataUsageChangeCallback()
{
    if (eventCallback_.callbackRef) {
        napi_delete_reference(callDataUsageCallback_.env, callDataUsageCallback_.callbackRef);
        napi_delete_reference(callDataUsageCallback_.env, callDataUsageCallback_.thisVar);
        (void)memset_s(&callDataUsageCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterCameraCapabilitiesChangeCallback(EventCallback eventCallback)
{
    cameraCapabilitiesCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterCameraCapabilitiesChangeCallback()
{
    if (eventCallback_.callbackRef) {
        napi_delete_reference(cameraCapabilitiesCallback_.env, cameraCapabilitiesCallback_.callbackRef);
        napi_delete_reference(cameraCapabilitiesCallback_.env, cameraCapabilitiesCallback_.thisVar);
        (void)memset_s(&cameraCapabilitiesCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterCallSessionEventChangeCallback(EventCallback eventCallback)
{
    callSessionEventCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterCallSessionEventChangeCallback()
{
    if (eventCallback_.callbackRef) {
        napi_delete_reference(callSessionEventCallback_.env, callSessionEventCallback_.callbackRef);
        napi_delete_reference(callSessionEventCallback_.env, callSessionEventCallback_.thisVar);
        (void)memset_s(&callSessionEventCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterDisconnectedCauseCallback(EventCallback eventCallback)
{
    callDisconnectCauseCallback_ = eventCallback;
}

void NapiCallAbilityCallback::UnRegisterDisconnectedCauseCallback()
{
    if (callDisconnectCauseCallback_.callbackRef) {
        napi_delete_reference(callDisconnectCauseCallback_.env, callDisconnectCauseCallback_.callbackRef);
        napi_delete_reference(callDisconnectCauseCallback_.env, callDisconnectCauseCallback_.thisVar);
        (void)memset_s(&callDisconnectCauseCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

void NapiCallAbilityCallback::RegisterCallOttRequestCallback(EventCallback ottRequestCallback)
{
    ottRequestCallback_ = ottRequestCallback;
}

void NapiCallAbilityCallback::UnRegisterCallOttRequestCallback()
{
    if (ottRequestCallback_.callbackRef) {
        napi_delete_reference(ottRequestCallback_.env, ottRequestCallback_.callbackRef);
        napi_delete_reference(ottRequestCallback_.env, ottRequestCallback_.thisVar);
        (void)memset_s(&ottRequestCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
}

int32_t NapiCallAbilityCallback::RegisterGetWaitingCallback(EventCallback callback)
{
    std::lock_guard<std::mutex> lock(getWaitingCallbackMutex_);
    if (getWaitingCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    getWaitingCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterGetWaitingCallback()
{
    std::lock_guard<std::mutex> lock(getWaitingCallbackMutex_);
    (void)memset_s(&getWaitingCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterCloseUnFinishedUssdCallback(EventCallback callback)
{
    std::lock_guard<std::mutex> lock(closeUnfinishedUssdCallbackMutex_);
    if (closeUnfinishedUssdCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    closeUnfinishedUssdCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterCloseUnFinishedUssdCallback()
{
    std::lock_guard<std::mutex> lock(closeUnfinishedUssdCallbackMutex_);
    (void)memset_s(&closeUnfinishedUssdCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterSetWaitingCallback(EventCallback callback)
{
    std::lock_guard<std::mutex> lock(setWaitingCallbackMutex_);
    if (setWaitingCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    setWaitingCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterSetWaitingCallback()
{
    std::lock_guard<std::mutex> lock(setWaitingCallbackMutex_);
    (void)memset_s(&setWaitingCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterGetRestrictionCallback(EventCallback callback)
{
    std::lock_guard<std::mutex> lock(getRestrictionCallbackMutex_);
    if (getRestrictionCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    getRestrictionCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterGetRestrictionCallback()
{
    std::lock_guard<std::mutex> lock(getRestrictionCallbackMutex_);
    (void)memset_s(&getRestrictionCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterSetRestrictionCallback(EventCallback callback)
{
    if (setRestrictionCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    setRestrictionCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterSetRestrictionCallback()
{
    (void)memset_s(&setRestrictionCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterSetRestrictionPasswordCallback(EventCallback callback)
{
    std::lock_guard<std::mutex> lock(setRestrictionPasswordCallbackMutex_);
    if (setRestrictionPasswordCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    setRestrictionPasswordCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterSetRestrictionPasswordCallback()
{
    std::lock_guard<std::mutex> lock(setRestrictionPasswordCallbackMutex_);
    (void)memset_s(&setRestrictionPasswordCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterGetTransferCallback(EventCallback callback, int32_t type)
{
    std::lock_guard<std::mutex> lock(getTransferCallbackMutex_);
    getTransferCallback_ = callback;
    getCallTransferReason_ = type;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterGetTransferCallback()
{
    std::lock_guard<std::mutex> lock(getTransferCallbackMutex_);
    (void)memset_s(&getTransferCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterSetTransferCallback(EventCallback callback)
{
    std::lock_guard<std::mutex> lock(setTransferCallbackMutex_);
    if (setTransferCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    setTransferCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterSetTransferCallback()
{
    std::lock_guard<std::mutex> lock(setTransferCallbackMutex_);
    (void)memset_s(&setTransferCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterStartRttCallback(EventCallback callback)
{
    if (startRttCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    startRttCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterStartRttCallback()
{
    (void)memset_s(&startRttCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::RegisterStopRttCallback(EventCallback callback)
{
    if (stopRttCallback_.thisVar) {
        TELEPHONY_LOGE("callback already exist!");
        return CALL_ERR_CALLBACK_ALREADY_EXIST;
    }
    stopRttCallback_ = callback;
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::UnRegisterStopRttCallback()
{
    (void)memset_s(&stopRttCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
}

int32_t NapiCallAbilityCallback::ReportStartRttInfo(AppExecFwk::PacMap &resultInfo)
{
    if (startRttCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("startRttCallback_ is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(startRttCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = startRttCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportStartRttInfo uv_queue_work_with_qos");
        }, ReportStartRttInfoWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (startRttCallback_.thisVar) {
        (void)memset_s(&startRttCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportStopRttInfo(AppExecFwk::PacMap &resultInfo)
{
    if (stopRttCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("startRttCallback_ is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(stopRttCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = stopRttCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t resultCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportStopRttInfo uv_queue_work_with_qos");
        }, ReportStopRttInfoWork, uv_qos_default);
    if (resultCode != 0) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("failed to add uv_queue_work_with_qos, resultCode: %{public}d", resultCode);
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (stopRttCallback_.thisVar) {
        (void)memset_s(&stopRttCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdateCallStateInfo(const CallAttributeInfo &info)
{
    if (stateCallback_.thisVar == nullptr) {
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    auto callStateWorker = std::make_shared<CallStateWorker>();
    callStateWorker->info = info;
    callStateWorker->callback = stateCallback_;
    auto task = [callStateWorker]() {
        ReportCallState(callStateWorker->info, callStateWorker->callback);
    };
    if (napi_status::napi_ok != napi_send_event(stateCallback_.env, task, napi_eprio_high)) {
        TELEPHONY_LOGE("napi_send_event: Failed to Send UpdateCallStateInfo Event");
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdateMeeTimeStateInfo(const CallAttributeInfo &info)
{
    if (meeTimeStateCallback_.thisVar == nullptr) {
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    auto meeTimeStateWorker = std::make_shared<MeeTimeStateWorker>();
    meeTimeStateWorker->info = info;
    meeTimeStateWorker->callback = meeTimeStateCallback_;
    auto task = [meeTimeStateWorker]() {
        ReportCallState(meeTimeStateWorker->info, meeTimeStateWorker->callback);
    };
    if (napi_status::napi_ok != napi_send_event(meeTimeStateCallback_.env, task, napi_eprio_high)) {
        TELEPHONY_LOGE("napi_send_event: Failed to Send UpdateMeeTimeStateInfo Event");
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

/**
 * To notify an application of a call status change, register a callback with on() first.
 */
int32_t NapiCallAbilityCallback::ReportCallState(CallAttributeInfo &info, EventCallback stateCallback)
{
    napi_env env = stateCallback.env;
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    if (scope == nullptr) {
        TELEPHONY_LOGE("scope is nullptr");
        napi_close_handle_scope(env, scope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, callbackValues[ARRAY_INDEX_FIRST], "accountNumber", info.accountNumber);
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "accountId", info.accountId);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "videoState", static_cast<int32_t>(info.videoState));
    NapiCallManagerUtils::SetPropertyInt64(env, callbackValues[ARRAY_INDEX_FIRST], "startTime", info.startTime);
    NapiCallManagerUtils::SetPropertyBoolean(env, callbackValues[ARRAY_INDEX_FIRST], "isEcc", info.isEcc);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "callType", static_cast<int32_t>(info.callType));
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "callId", info.callId);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "callState", static_cast<int32_t>(info.callState));
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "conferenceState", static_cast<int32_t>(info.conferenceState));
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "crsType", info.crsType);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "originalCallType", info.originalCallType);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "phoneOrWatch", info.phoneOrWatch);
    napi_set_named_property(env, callbackValues[ARRAY_INDEX_FIRST], std::string("extraParams").c_str(),
        AppExecFwk::WrapWantParams(env, AAFwk::WantParamWrapper::ParseWantParamsWithBrackets(info.extraParamsString)));
    ReportCallAttribute(env, callbackValues, ARRAY_INDEX_THIRD, info);
    napi_get_reference_value(env, stateCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, scope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, stateCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, scope);
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportCallAttribute(napi_env &env, napi_value callbackValues[],
    const size_t callbackValuesCount, CallAttributeInfo &info)
{
    std::string str(info.numberLocation);
    if (str == "default") {
        TELEPHONY_LOGE("numberLocation is default");
        (void)memset_s(info.numberLocation, kMaxNumberLen, 0, kMaxNumberLen);
    }
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, callbackValues[ARRAY_INDEX_FIRST], "numberLocation", info.numberLocation);
    TELEPHONY_LOGI("ReportCallState crsType = %{public}d", info.crsType);
    if (info.callType == CallType::TYPE_VOIP) {
        napi_value voipObject = nullptr;
        CreateVoipNapiValue(env, voipObject, info);
        napi_set_named_property(env, callbackValues[ARRAY_INDEX_FIRST], "voipCallAttribute", voipObject);
    }
    napi_value markInfoObject = nullptr;
    CreateMarkInfoNapiValue(env, markInfoObject, info);
    napi_set_named_property(env, callbackValues[ARRAY_INDEX_FIRST], "numberMarkInfo", markInfoObject);
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, callbackValues[ARRAY_INDEX_FIRST], "distributedContactName", info.contactName);
}

void NapiCallAbilityCallback::CreateVoipNapiValue(napi_env &env, napi_value &voipObject, CallAttributeInfo &info)
{
    napi_create_object(env, &voipObject);
    NapiCallManagerUtils::SetPropertyStringUtf8(env, voipObject, "userName", info.voipCallInfo.userName);
    NapiCallManagerUtils::SetPropertyStringUtf8(env, voipObject, "abilityName", info.voipCallInfo.abilityName);
    NapiCallManagerUtils::SetPropertyStringUtf8(env, voipObject, "extensionId", info.voipCallInfo.extensionId);
    NapiCallManagerUtils::SetPropertyStringUtf8(env, voipObject, "voipBundleName", info.voipCallInfo.voipBundleName);
    NapiCallManagerUtils::SetPropertyStringUtf8(env, voipObject, "voipCallId", info.voipCallInfo.voipCallId);
    NapiCallManagerUtils::SetPropertyBoolean(env, voipObject, "showBannerForIncomingCall",
        info.voipCallInfo.showBannerForIncomingCall);
    NapiCallManagerUtils::SetPropertyBoolean(env, voipObject, "isConferenceCall", info.voipCallInfo.isConferenceCall);
    NapiCallManagerUtils::SetPropertyBoolean(
        env, voipObject, "isVoiceAnswerSupported", info.voipCallInfo.isVoiceAnswerSupported);
    NapiCallManagerUtils::SetPropertyBoolean(env, voipObject, "hasMicPermission", info.voipCallInfo.hasMicPermission);
    NapiCallManagerUtils::SetPropertyBoolean(env, voipObject, "isCapsuleSticky", info.voipCallInfo.isCapsuleSticky);
    NapiCallManagerUtils::SetPropertyInt32(env, voipObject, "uid", info.voipCallInfo.uid);
    std::shared_ptr<Media::PixelMap> userProfile =
        std::shared_ptr<Media::PixelMap>(Media::PixelMap::DecodeTlv(info.voipCallInfo.userProfile));
    napi_value pixelMapObject = Media::PixelMapNapi::CreatePixelMap(env, userProfile);
    napi_set_named_property(env, voipObject, "userProfile", pixelMapObject);
}

void NapiCallAbilityCallback::CreateMarkInfoNapiValue(napi_env &env,
    napi_value &markInfoObject, CallAttributeInfo &info)
{
    napi_create_object(env, &markInfoObject);
    NapiCallManagerUtils::SetPropertyInt32(env, markInfoObject, "markType",
        static_cast<int32_t>(info.numberMarkInfo.markType));
    NapiCallManagerUtils::SetPropertyStringUtf8(env, markInfoObject, "markContent", info.numberMarkInfo.markContent);
    NapiCallManagerUtils::SetPropertyInt32(env, markInfoObject, "markCount",
        static_cast<int32_t>(info.numberMarkInfo.markCount));
    NapiCallManagerUtils::SetPropertyStringUtf8(env, markInfoObject, "markSource", info.numberMarkInfo.markSource);
    NapiCallManagerUtils::SetPropertyBoolean(env, markInfoObject, "isCloud", info.numberMarkInfo.isCloud);
    NapiCallManagerUtils::SetPropertyStringUtf8(env, markInfoObject, "markDetails", info.numberMarkInfo.markDetails);
}

int32_t NapiCallAbilityCallback::UpdateCallEvent(const CallEventInfo &info)
{
    if (eventCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("eventCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(eventCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallEventWorker *callEventWorker = std::make_unique<CallEventWorker>().release();
    if (callEventWorker == nullptr) {
        TELEPHONY_LOGE("callEventWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callEventWorker->info = info;
    callEventWorker->callback = eventCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callEventWorker;
        callEventWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callEventWorker;
    int32_t errCode = uv_queue_work_with_qos(loop, work, [](uv_work_t *work) {
        TELEPHONY_LOGD("UpdateCallEvent uv_queue_work_with_qos");
    }, ReportCallEventWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete callEventWorker;
        callEventWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportCallEventWork(uv_work_t *work, int32_t status)
{
    CallEventWorker *dataWorkerData = (CallEventWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportCallEvent(dataWorkerData->info, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportCallEvent results %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportCallEvent(CallEventInfo &info, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope scopeCallEvent = nullptr;
    napi_open_handle_scope(env, &scopeCallEvent);
    if (scopeCallEvent == nullptr) {
        TELEPHONY_LOGE("scopeCallEvent is nullptr");
        napi_close_handle_scope(env, scopeCallEvent);
        return TELEPHONY_ERROR;
    }
    napi_value callEventCallbackFunc = nullptr;
    napi_value callEventCallbackValues[ARRAY_INDEX_THIRD] = { 0 };
    napi_create_object(env, &callEventCallbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callEventCallbackValues[ARRAY_INDEX_FIRST], "eventId", static_cast<int32_t>(info.eventId));
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, callEventCallbackValues[ARRAY_INDEX_FIRST], "accountNumber", info.phoneNum);
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, callEventCallbackValues[ARRAY_INDEX_FIRST], "bundleName", info.bundleName);
    napi_get_reference_value(env, eventCallback.callbackRef, &callEventCallbackFunc);
    if (callEventCallbackFunc == nullptr) {
        TELEPHONY_LOGE("callEventCallbackFunc is null!");
        napi_close_handle_scope(env, scopeCallEvent);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callEventCallbackFunc, DATA_LENGTH_ONE, callEventCallbackValues, &callbackResult);
    napi_close_handle_scope(env, scopeCallEvent);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdateCallDisconnectedCause(const DisconnectedDetails &details)
{
    if (callDisconnectCauseCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("callDisconnectCauseCallback_ is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(callDisconnectCauseCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallDisconnectedCauseWorker *callDisconnectedCauseWorker =
        std::make_unique<CallDisconnectedCauseWorker>().release();
    if (callDisconnectedCauseWorker == nullptr) {
        TELEPHONY_LOGE("callDisconnectedCauseWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callDisconnectedCauseWorker->details = details;
    callDisconnectedCauseWorker->callback = callDisconnectCauseCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callDisconnectedCauseWorker;
        callDisconnectedCauseWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callDisconnectedCauseWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("UpdateCallDisconnectedCause uv_queue_work_with_qos");
        }, ReportCallDisconnectedCauseWork, uv_qos_default);
    if (errCode != 0) {
        delete callDisconnectedCauseWorker;
        callDisconnectedCauseWorker = nullptr;
        TELEPHONY_LOGE("failed to add uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (callDisconnectCauseCallback_.thisVar) {
        (void)memset_s(&callDisconnectCauseCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportCallDisconnectedCauseWork(uv_work_t *work, int32_t status)
{
    CallDisconnectedCauseWorker *dataWorkerData = (CallDisconnectedCauseWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportDisconnectedCause(dataWorkerData->details, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportDisconnectedCause results %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportDisconnectedCause(
    const DisconnectedDetails &details, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope disconnectedScope = nullptr;
    napi_open_handle_scope(env, &disconnectedScope);
    if (disconnectedScope == nullptr) {
        TELEPHONY_LOGE("disconnectedScope is nullptr");
        napi_close_handle_scope(env, disconnectedScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "reason", static_cast<int32_t>(details.reason));
    NapiCallManagerUtils::SetPropertyStringUtf8(env, callbackValues[ARRAY_INDEX_FIRST], "message", details.message);
    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, disconnectedScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, disconnectedScope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdateAsyncResultsInfo(
    const CallResultReportId reportId, AppExecFwk::PacMap &resultInfo)
{
    int32_t result = TELEPHONY_ERR_FAIL;
    TELEPHONY_LOGI("UpdateAsyncResultsInfo reportId = %{public}d", reportId);
    auto itFunc = memberFuncMap_.find(reportId);
    if (itFunc != memberFuncMap_.end() && itFunc->second != nullptr) {
        auto memberFunc = itFunc->second;
        result = memberFunc(resultInfo);
    }
    return result;
}

int32_t NapiCallAbilityCallback::UpdateMmiCodeResultsInfo(const MmiCodeInfo &info)
{
    if (mmiCodeCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("mmiCodeCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(mmiCodeCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    MmiCodeWorker *mmiCodeWorker = std::make_unique<MmiCodeWorker>().release();
    if (mmiCodeWorker == nullptr) {
        TELEPHONY_LOGE("mmiCodeWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    mmiCodeWorker->info = info;
    mmiCodeWorker->callback = mmiCodeCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete mmiCodeWorker;
        mmiCodeWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)mmiCodeWorker;
    int32_t errCode = uv_queue_work_with_qos(loop, work, [](uv_work_t *work) {
        TELEPHONY_LOGD("UpdateMmiCodeResultsInfo uv_queue_work_with_qos");
    }, ReportMmiCodeWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete mmiCodeWorker;
        mmiCodeWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportMmiCodeWork(uv_work_t *work, int32_t status)
{
    MmiCodeWorker *dataWorkerData = (MmiCodeWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportMmiCode(dataWorkerData->info, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportMmiCode result = %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

/**
 * To notify an application of MMI code result, register a callback with on() first.
 */
int32_t NapiCallAbilityCallback::ReportMmiCode(MmiCodeInfo &info, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope mmiCodeScope = nullptr;
    napi_open_handle_scope(env, &mmiCodeScope);
    if (mmiCodeScope == nullptr) {
        TELEPHONY_LOGE("mmiCodeScope is nullptr");
        napi_close_handle_scope(env, mmiCodeScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_FOURTH] = { 0 };
    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "result", static_cast<int32_t>(info.result));
    NapiCallManagerUtils::SetPropertyStringUtf8(env, callbackValues[ARRAY_INDEX_FIRST], "message", info.message);
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "mmiCodeType",
        static_cast<int32_t>(info.mmiCodeType));
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "action",
        static_cast<int32_t>(info.action));
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "status",
        static_cast<int32_t>(info.status));
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "classCw",
        static_cast<int32_t>(info.classCw));
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "reason",
        static_cast<int32_t>(info.reason));
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "time",
        static_cast<int32_t>(info.time));
    NapiCallManagerUtils::SetPropertyStringUtf8(env, callbackValues[ARRAY_INDEX_FIRST], "number", info.number);
    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, mmiCodeScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, mmiCodeScope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdateAudioDeviceInfo(const AudioDeviceInfo &info)
{
    std::lock_guard<std::mutex> lock(audioDeviceCallbackMutex_);
    if (audioDeviceCallback_.thisVar == nullptr) {
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(audioDeviceCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    AudioDeviceWork *audioDeviceWork = std::make_unique<AudioDeviceWork>().release();
    if (audioDeviceWork == nullptr) {
        TELEPHONY_LOGE("audioDeviceWork is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    audioDeviceWork->info = info;
    audioDeviceWork->callback = audioDeviceCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete audioDeviceWork;
        audioDeviceWork = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)audioDeviceWork;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("UpdateAudioDeviceInfo uv_queue_work_with_qos");
        }, ReportAudioDeviceInfoWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete audioDeviceWork;
        audioDeviceWork = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportAudioDeviceInfoWork(uv_work_t *work, int32_t status)
{
    TELEPHONY_LOGI("report audio device info work.");
    std::lock_guard<std::mutex> lock(audioDeviceCallbackMutex_);
    AudioDeviceWork *dataWorkerData = (AudioDeviceWork *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    if (audioDeviceCallback_.thisVar && audioDeviceCallback_.callbackRef) {
        int32_t ret = ReportAudioDeviceInfo(dataWorkerData->info, audioDeviceCallback_);
        TELEPHONY_LOGI("ReportAudioDeviceInfo result = %{public}d", ret);
        delete dataWorkerData;
        dataWorkerData = nullptr;
        delete work;
        work = nullptr;
    }
}

int32_t NapiCallAbilityCallback::ReportAudioDeviceInfo(AudioDeviceInfo &info, EventCallback eventCallback)
{
    TELEPHONY_LOGI("report audio device info.");
    napi_env env = eventCallback.env;
    napi_handle_scope AudioDeviceInfoScope = nullptr;
    napi_open_handle_scope(env, &AudioDeviceInfoScope);
    if (AudioDeviceInfoScope == nullptr) {
        TELEPHONY_LOGE("AudioDeviceInfoScope is nullptr");
        napi_close_handle_scope(env, AudioDeviceInfoScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_SECOND] = { 0 };
    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);

    NapiCallManagerUtils::SetPropertyBoolean(env, callbackValues[ARRAY_INDEX_FIRST], "isMuted", info.isMuted);
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "callId", info.callId);

    napi_value currentAudioDeviceValue = nullptr;
    napi_create_object(env, &currentAudioDeviceValue);
    NapiCallManagerUtils::SetPropertyInt32(env, currentAudioDeviceValue, "deviceType",
        static_cast<int32_t>(info.currentAudioDevice.deviceType));
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, currentAudioDeviceValue, "address", info.currentAudioDevice.address);
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, currentAudioDeviceValue, "deviceName", info.currentAudioDevice.deviceName);
    napi_set_named_property(env, callbackValues[ARRAY_INDEX_FIRST], "currentAudioDevice", currentAudioDeviceValue);

    napi_value audioDeviceListValue = nullptr;
    napi_create_array(env, &audioDeviceListValue);
    std::vector<AudioDevice>::iterator it = info.audioDeviceList.begin();
    int32_t i = 0;
    for (; it != info.audioDeviceList.end(); ++it) {
        napi_value value = nullptr;
        napi_create_object(env, &value);
        NapiCallManagerUtils::SetPropertyInt32(env, value, "deviceType", static_cast<int32_t>(it->deviceType));
        NapiCallManagerUtils::SetPropertyStringUtf8(env, value, "address", it->address);
        NapiCallManagerUtils::SetPropertyStringUtf8(env, value, "deviceName", it->deviceName);
        napi_set_element(env, audioDeviceListValue, i, value);
        ++i;
    }
    napi_set_named_property(env, callbackValues[ARRAY_INDEX_FIRST], "audioDeviceList", audioDeviceListValue);
    if (eventCallback.callbackRef == nullptr) {
        TELEPHONY_LOGE("eventCallback callbackRef is null!");
        napi_close_handle_scope(env, AudioDeviceInfoScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, AudioDeviceInfoScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    if (eventCallback.thisVar == nullptr) {
        TELEPHONY_LOGE("eventCallback thisVar is null!");
        napi_close_handle_scope(env, AudioDeviceInfoScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, AudioDeviceInfoScope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::OttCallRequest(OttCallRequestId requestId, AppExecFwk::PacMap &info)
{
    if (ottRequestCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("stateCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(ottRequestCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallOttWorker *callOttWorker = std::make_unique<CallOttWorker>().release();
    if (callOttWorker == nullptr) {
        TELEPHONY_LOGE("callOttWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callOttWorker->requestId = requestId;
    callOttWorker->info = info;
    callOttWorker->callback = ottRequestCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callOttWorker;
        callOttWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callOttWorker;
    int32_t errCode = uv_queue_work_with_qos(loop, work, [](uv_work_t *work) {
        TELEPHONY_LOGD("OttCallRequest uv_queue_work_with_qos");
    }, ReportCallOttWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete callOttWorker;
        callOttWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportGetWaitingInfo(AppExecFwk::PacMap &resultInfo)
{
    std::lock_guard<std::mutex> lock(getWaitingCallbackMutex_);
    if (getWaitingCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("getWaitingCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(getWaitingCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorkerData = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorkerData == nullptr) {
        TELEPHONY_LOGE("callSupplementWorkerData is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorkerData->info = resultInfo;
    callSupplementWorkerData->callback = getWaitingCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorkerData;
        callSupplementWorkerData = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorkerData;
    int32_t errorCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportGetWaitingInfo uv_queue_work_with_qos");
        }, ReportWaitAndLimitInfoWork, uv_qos_default);
    if (errorCode != 0) {
        delete callSupplementWorkerData;
        callSupplementWorkerData = nullptr;
        delete work;
        work = nullptr;
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errorCode: %{public}d", errorCode);
        return TELEPHONY_ERROR;
    }
    if (getWaitingCallback_.thisVar) {
        (void)memset_s(&getWaitingCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportCloseUnFinishedUssdInfo(AppExecFwk::PacMap &resultInfo)
{
    std::lock_guard<std::mutex> lock(closeUnfinishedUssdCallbackMutex_);
    if (closeUnfinishedUssdCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("closeUnfinishedUssdCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(closeUnfinishedUssdCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementDataWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementDataWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementDataWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementDataWorker->info = resultInfo;
    callSupplementDataWorker->callback = closeUnfinishedUssdCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementDataWorker;
        callSupplementDataWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementDataWorker;
    int32_t errorCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportCloseUnFinishedUssdInfo uv_queue_work_with_qos");
        }, ReportExecutionResultWork, uv_qos_default);
    if (errorCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errorCode: %{public}d", errorCode);
        delete callSupplementDataWorker;
        callSupplementDataWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (closeUnfinishedUssdCallback_.thisVar) {
        (void)memset_s(&closeUnfinishedUssdCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportSetWaitingInfo(AppExecFwk::PacMap &resultInfo)
{
    std::lock_guard<std::mutex> lock(setWaitingCallbackMutex_);
    if (setWaitingCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("setWaitingCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(setWaitingCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = setWaitingCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t resultCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportSetWaitingInfo uv_queue_work_with_qos");
        }, ReportExecutionResultWork, uv_qos_default);
    if (resultCode != 0) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        delete work;
        work = nullptr;
        TELEPHONY_LOGE("failed to add uv_queue_work_with_qos, resultCode: %{public}d", resultCode);
        return TELEPHONY_ERROR;
    }
    if (setWaitingCallback_.thisVar) {
        (void)memset_s(&setWaitingCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportGetRestrictionInfo(AppExecFwk::PacMap &resultInfo)
{
    std::lock_guard<std::mutex> lock(getRestrictionCallbackMutex_);
    if (getRestrictionCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("getRestrictionCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(getRestrictionCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = getRestrictionCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t resultCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportGetRestrictionInfo uv_queue_work_with_qos");
        }, ReportWaitAndLimitInfoWork, uv_qos_default);
    if (resultCode != 0) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, resultCode: %{public}d", resultCode);
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (getRestrictionCallback_.thisVar) {
        (void)memset_s(&getRestrictionCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportSetRestrictionInfo(AppExecFwk::PacMap &resultInfo)
{
    if (setRestrictionCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("setRestrictionCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(setRestrictionCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = setRestrictionCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t resultCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportSetRestrictionInfo uv_queue_work_with_qos");
        }, ReportExecutionResultWork, uv_qos_default);
    if (resultCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, resultCode: %{public}d", resultCode);
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (setRestrictionCallback_.thisVar) {
        (void)memset_s(&setRestrictionCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportSetRestrictionPassword(AppExecFwk::PacMap &resultInfo)
{
    std::lock_guard<std::mutex> lock(setRestrictionPasswordCallbackMutex_);
    if (setRestrictionPasswordCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("setRestrictionPasswordCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(setRestrictionPasswordCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = setRestrictionPasswordCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportSetRestrictionPassword uv_queue_work_with_qos");
        }, ReportExecutionResultWork, uv_qos_default);
    if (errCode != 0) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        delete work;
        work = nullptr;
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        return TELEPHONY_ERROR;
    }
    if (setRestrictionPasswordCallback_.thisVar) {
        (void)memset_s(&setRestrictionPasswordCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportGetTransferInfo(AppExecFwk::PacMap &resultInfo)
{
    std::lock_guard<std::mutex> lock(getTransferCallbackMutex_);
    if (getTransferCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("getTransferCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    if (getCallTransferReason_ != resultInfo.GetIntValue("reason")) {
        TELEPHONY_LOGE("Transfer reason is different, require is %{public}d, now is %{public}d", getCallTransferReason_,
            resultInfo.GetIntValue("reason"));
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(getTransferCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = getTransferCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportGetTransferInfo uv_queue_work_with_qos");
        }, ReportSupplementInfoWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed add to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (getTransferCallback_.thisVar) {
        (void)memset_s(&getTransferCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::ReportSetTransferInfo(AppExecFwk::PacMap &resultInfo)
{
    std::lock_guard<std::mutex> lock(setTransferCallbackMutex_);
    if (setTransferCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("setTransferCallback is null!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(setTransferCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSupplementWorker *callSupplementWorker = std::make_unique<CallSupplementWorker>().release();
    if (callSupplementWorker == nullptr) {
        TELEPHONY_LOGE("callSupplementWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSupplementWorker->info = resultInfo;
    callSupplementWorker->callback = setTransferCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSupplementWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("ReportSetTransferInfo uv_queue_work_with_qos");
        }, ReportExecutionResultWork, uv_qos_default);
    if (errCode != 0) {
        delete callSupplementWorker;
        callSupplementWorker = nullptr;
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    if (setTransferCallback_.thisVar) {
        (void)memset_s(&setTransferCallback_, sizeof(EventCallback), 0, sizeof(EventCallback));
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportWaitAndLimitInfoWork(uv_work_t *work, int32_t status)
{
    CallSupplementWorker *dataWorkerData = (CallSupplementWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    ReportWaitAndLimitInfo(dataWorkerData->info, dataWorkerData->callback);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

void NapiCallAbilityCallback::ReportWaitAndLimitInfo(AppExecFwk::PacMap &resultInfo, EventCallback supplementInfo)
{
    napi_env env = supplementInfo.env;
    int32_t result = resultInfo.GetIntValue("result");
    int32_t status = resultInfo.GetIntValue("status");
    napi_handle_scope limitScope = nullptr;
    napi_open_handle_scope(env, &limitScope);
    if (limitScope == nullptr) {
        TELEPHONY_LOGE("limitScope is nullptr");
        napi_close_handle_scope(env, limitScope);
        return;
    }
    if (supplementInfo.callbackRef != nullptr) {
        napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
        if (result == TELEPHONY_SUCCESS) {
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateUndefined(env);
            napi_create_int32(env, status, &callbackValues[ARRAY_INDEX_SECOND]);
        } else {
            std::string errTip = std::to_string(CALL_ERR_NAPI_INTERFACE_FAILED);
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateErrorMessage(env, errTip);
            callbackValues[ARRAY_INDEX_SECOND] = NapiCallManagerUtils::CreateUndefined(env);
        }
        napi_value callbackFunc = nullptr;
        napi_get_reference_value(env, supplementInfo.callbackRef, &callbackFunc);
        napi_value thisVar = nullptr;
        napi_get_reference_value(env, supplementInfo.thisVar, &thisVar);
        napi_value callbackResult = nullptr;
        napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_TWO, callbackValues, &callbackResult);
        napi_delete_reference(env, supplementInfo.callbackRef);
        napi_delete_reference(env, supplementInfo.thisVar);
    } else if (supplementInfo.deferred != nullptr) {
        if (result == TELEPHONY_SUCCESS) {
            napi_value promiseValue = nullptr;
            napi_create_int32(env, status, &promiseValue);
            napi_resolve_deferred(env, supplementInfo.deferred, promiseValue);
        } else {
            std::string errTip = std::to_string(CALL_ERR_NAPI_INTERFACE_FAILED);
            napi_reject_deferred(env, supplementInfo.deferred, NapiCallManagerUtils::CreateErrorMessage(env, errTip));
        }
    }
    napi_close_handle_scope(env, limitScope);
}

void NapiCallAbilityCallback::ReportSupplementInfoWork(uv_work_t *work, int32_t status)
{
    CallSupplementWorker *dataWorkerData = (CallSupplementWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    ReportSupplementInfo(dataWorkerData->info, dataWorkerData->callback);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

void NapiCallAbilityCallback::ReportSupplementInfo(AppExecFwk::PacMap &resultInfo, EventCallback supplementInfo)
{
    napi_env env = supplementInfo.env;
    napi_handle_scope supplementScope = nullptr;
    napi_open_handle_scope(env, &supplementScope);
    if (supplementScope == nullptr) {
        TELEPHONY_LOGE("supplementScope is nullptr");
        napi_close_handle_scope(env, supplementScope);
        return;
    }
    napi_value callbackValue = nullptr;
    napi_create_object(env, &callbackValue);
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValue, "status", resultInfo.GetIntValue("status"));
    NapiCallManagerUtils::SetPropertyStringUtf8(env, callbackValue, "number", resultInfo.GetStringValue("number"));
    int32_t result = resultInfo.GetIntValue("result");
    if (supplementInfo.callbackRef != nullptr) {
        napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
        if (result == TELEPHONY_SUCCESS) {
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateUndefined(env);
            callbackValues[ARRAY_INDEX_SECOND] = callbackValue;
        } else {
            JsError error = NapiUtil::ConverErrorMessageForJs(result);
            callbackValues[ARRAY_INDEX_FIRST] =
                NapiCallManagerUtils::CreateErrorMessageWithErrorCode(env, error.errorMessage, error.errorCode);
            callbackValues[ARRAY_INDEX_SECOND] = NapiCallManagerUtils::CreateUndefined(env);
        }
        napi_value callbackFunc = nullptr;
        napi_get_reference_value(env, supplementInfo.callbackRef, &callbackFunc);
        napi_value thisVar = nullptr;
        napi_get_reference_value(env, supplementInfo.thisVar, &thisVar);
        napi_value callbackResult = nullptr;
        napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_TWO, callbackValues, &callbackResult);
        napi_delete_reference(env, supplementInfo.callbackRef);
        napi_delete_reference(env, supplementInfo.thisVar);
    } else if (supplementInfo.deferred != nullptr) {
        if (result == TELEPHONY_SUCCESS) {
            napi_resolve_deferred(env, supplementInfo.deferred, callbackValue);
        } else {
            JsError error = NapiUtil::ConverErrorMessageForJs(result);
            napi_reject_deferred(env, supplementInfo.deferred,
                NapiCallManagerUtils::CreateErrorMessageWithErrorCode(env, error.errorMessage, error.errorCode));
        }
    }
    napi_close_handle_scope(env, supplementScope);
}

void NapiCallAbilityCallback::ReportExecutionResultWork(uv_work_t *work, int32_t status)
{
    CallSupplementWorker *dataWorkerData = (CallSupplementWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    ReportExecutionResult(dataWorkerData->callback, dataWorkerData->info);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

void NapiCallAbilityCallback::ReportExecutionResult(EventCallback &settingInfo, AppExecFwk::PacMap &resultInfo)
{
    napi_env env = settingInfo.env;
    napi_handle_scope executionScope = nullptr;
    napi_open_handle_scope(env, &executionScope);
    if (executionScope == nullptr) {
        TELEPHONY_LOGE("executionScope is nullptr");
        napi_close_handle_scope(env, executionScope);
        return;
    }
    napi_value callbackValue = nullptr;
    napi_create_object(env, &callbackValue);
    int32_t result = resultInfo.GetIntValue("result");
    if (settingInfo.callbackRef != nullptr) {
        napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
        if (result == TELEPHONY_SUCCESS) {
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateUndefined(env);
            napi_get_null(env, &callbackValues[ARRAY_INDEX_SECOND]);
        } else {
            JsError error = NapiUtil::ConverErrorMessageForJs(result);
            callbackValues[ARRAY_INDEX_FIRST] =
                NapiCallManagerUtils::CreateErrorMessageWithErrorCode(env, error.errorMessage, error.errorCode);
            callbackValues[ARRAY_INDEX_SECOND] = NapiCallManagerUtils::CreateUndefined(env);
        }
        napi_value callbackFunc = nullptr;
        napi_get_reference_value(env, settingInfo.callbackRef, &callbackFunc);
        napi_value thisVar = nullptr;
        napi_get_reference_value(env, settingInfo.thisVar, &thisVar);
        napi_value callbackResult = nullptr;
        napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_TWO, callbackValues, &callbackResult);
        napi_delete_reference(env, settingInfo.callbackRef);
        napi_delete_reference(env, settingInfo.thisVar);
    } else if (settingInfo.deferred != nullptr) {
        if (result == TELEPHONY_SUCCESS) {
            napi_value promiseValue = nullptr;
            napi_get_null(env, &promiseValue);
            napi_resolve_deferred(env, settingInfo.deferred, promiseValue);
        } else {
            JsError error = NapiUtil::ConverErrorMessageForJs(result);
            napi_reject_deferred(env, settingInfo.deferred,
                NapiCallManagerUtils::CreateErrorMessageWithErrorCode(env, error.errorMessage, error.errorCode));
        }
    }
    napi_close_handle_scope(env, executionScope);
}

void NapiCallAbilityCallback::ReportStartRttInfoWork(uv_work_t *work, int32_t status)
{
    CallSupplementWorker *dataWorkerData = (CallSupplementWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    ReportStartRttInfo(dataWorkerData->info, dataWorkerData->callback);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

void NapiCallAbilityCallback::ReportStartRttInfo(AppExecFwk::PacMap &resultInfo, EventCallback supplementInfo)
{
    napi_env env = supplementInfo.env;
    int32_t result = resultInfo.GetIntValue("result");
    napi_handle_scope startRttScope = nullptr;
    napi_open_handle_scope(env, &startRttScope);
    if (startRttScope == nullptr) {
        TELEPHONY_LOGE("startRttScope is nullptr");
        napi_close_handle_scope(env, startRttScope);
        return;
    }
    if (supplementInfo.callbackRef != nullptr) {
        napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
        if (result == TELEPHONY_SUCCESS) {
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateUndefined(env);
        } else {
            std::string errTip = std::to_string(CALL_ERR_NAPI_INTERFACE_FAILED);
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateErrorMessage(env, errTip);
            callbackValues[ARRAY_INDEX_SECOND] = NapiCallManagerUtils::CreateUndefined(env);
        }
        napi_value callbackFunc = nullptr;
        napi_get_reference_value(env, supplementInfo.callbackRef, &callbackFunc);
        napi_value thisVar = nullptr;
        napi_get_reference_value(env, supplementInfo.thisVar, &thisVar);
        napi_value callbackResult = nullptr;
        napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_TWO, callbackValues, &callbackResult);
        napi_delete_reference(env, supplementInfo.callbackRef);
        napi_delete_reference(env, supplementInfo.thisVar);
    } else if (supplementInfo.deferred != nullptr) {
        if (result == TELEPHONY_SUCCESS) {
            napi_value promiseValue = nullptr;
            napi_get_null(env, &promiseValue);
            napi_resolve_deferred(env, supplementInfo.deferred, promiseValue);
        } else {
            std::string errTip = std::to_string(CALL_ERR_NAPI_INTERFACE_FAILED);
            napi_reject_deferred(env, supplementInfo.deferred, NapiCallManagerUtils::CreateErrorMessage(env, errTip));
        }
    }
    napi_close_handle_scope(env, startRttScope);
}

void NapiCallAbilityCallback::ReportStopRttInfoWork(uv_work_t *work, int32_t status)
{
    CallSupplementWorker *dataWorkerData = (CallSupplementWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    ReportStopRttInfo(dataWorkerData->info, dataWorkerData->callback);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

void NapiCallAbilityCallback::ReportStopRttInfo(AppExecFwk::PacMap &resultInfo, EventCallback supplementInfo)
{
    napi_env env = supplementInfo.env;
    int32_t result = resultInfo.GetIntValue("result");
    napi_handle_scope stopRttScope = nullptr;
    napi_open_handle_scope(env, &stopRttScope);
    if (stopRttScope == nullptr) {
        TELEPHONY_LOGE("stopRttScope is nullptr");
        napi_close_handle_scope(env, stopRttScope);
        return;
    }
    if (supplementInfo.callbackRef != nullptr) {
        napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
        if (result == TELEPHONY_SUCCESS) {
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateUndefined(env);
        } else {
            std::string errTip = std::to_string(CALL_ERR_NAPI_INTERFACE_FAILED);
            callbackValues[ARRAY_INDEX_FIRST] = NapiCallManagerUtils::CreateErrorMessage(env, errTip);
            callbackValues[ARRAY_INDEX_SECOND] = NapiCallManagerUtils::CreateUndefined(env);
        }
        napi_value callbackFunc = nullptr;
        napi_get_reference_value(env, supplementInfo.callbackRef, &callbackFunc);
        napi_value thisVar = nullptr;
        napi_get_reference_value(env, supplementInfo.thisVar, &thisVar);
        napi_value callbackResult = nullptr;
        napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_TWO, callbackValues, &callbackResult);
        napi_delete_reference(env, supplementInfo.callbackRef);
        napi_delete_reference(env, supplementInfo.thisVar);
    } else if (supplementInfo.deferred != nullptr) {
        if (result == TELEPHONY_SUCCESS) {
            napi_value promiseValue = nullptr;
            napi_get_null(env, &promiseValue);
            napi_resolve_deferred(env, supplementInfo.deferred, promiseValue);
        } else {
            std::string errTip = std::to_string(CALL_ERR_NAPI_INTERFACE_FAILED);
            napi_reject_deferred(env, supplementInfo.deferred, NapiCallManagerUtils::CreateErrorMessage(env, errTip));
        }
    }
    napi_close_handle_scope(env, stopRttScope);
}

void NapiCallAbilityCallback::ReportCallOttWork(uv_work_t *work, int32_t status)
{
    CallOttWorker *dataWorkerData = (CallOttWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    ReportCallOtt(dataWorkerData->callback, dataWorkerData->info, dataWorkerData->requestId);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportCallOtt(
    EventCallback &settingInfo, AppExecFwk::PacMap &resultInfo, OttCallRequestId requestId)
{
    napi_env env = settingInfo.env;
    napi_handle_scope callOttscope = nullptr;
    napi_open_handle_scope(env, &callOttscope);
    if (callOttscope == nullptr) {
        TELEPHONY_LOGE("callOttscope is nullptr");
        napi_close_handle_scope(env, callOttscope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_THIRD] = { 0 };
    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "requestId", static_cast<int32_t>(requestId));
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, callbackValues[ARRAY_INDEX_FIRST], "phoneNumber", resultInfo.GetStringValue("phoneNumber").c_str());
    NapiCallManagerUtils::SetPropertyStringUtf8(
        env, callbackValues[ARRAY_INDEX_FIRST], "bundleName", resultInfo.GetStringValue("bundleName").c_str());
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "videoState", resultInfo.GetIntValue("videoState"));
    switch (requestId) {
        case OttCallRequestId::OTT_REQUEST_INVITE_TO_CONFERENCE:
            NapiCallManagerUtils::SetPropertyStringUtf8(
                env, callbackValues[ARRAY_INDEX_FIRST], "number", resultInfo.GetStringValue("number").c_str());
            break;
        case OttCallRequestId::OTT_REQUEST_UPDATE_CALL_MEDIA_MODE:
            NapiCallManagerUtils::SetPropertyInt32(
                env, callbackValues[ARRAY_INDEX_FIRST], "callMediaMode", resultInfo.GetIntValue("callMediaMode"));
            break;
        default:
            break;
    }

    napi_get_reference_value(env, settingInfo.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, callOttscope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, settingInfo.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, callOttscope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdatePostDialDelay(const std::string str)
{
    if (postDialDelayCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("thisVar is nullptr!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(postDialDelayCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    PostDialDelayWorker *postDialDelayWorker = std::make_unique<PostDialDelayWorker>().release();
    if (postDialDelayWorker == nullptr) {
        TELEPHONY_LOGE("postDialDelayWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    postDialDelayWorker->postDialStr = str;
    postDialDelayWorker->callback = postDialDelayCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete postDialDelayWorker;
        postDialDelayWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)postDialDelayWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("UpdatePostDialDelay uv_queue_work_with_qos");
        }, ReportPostDialDelayWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete postDialDelayWorker;
        postDialDelayWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportPostDialDelayWork(uv_work_t *work, int32_t status)
{
    PostDialDelayWorker *dataWorkerData = (PostDialDelayWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportPostDialDelay(dataWorkerData->postDialStr, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportPostDialDelay result = %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportPostDialDelay(std::string postDialStr, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    if (scope == nullptr) {
        TELEPHONY_LOGE("scope is nullptr");
        napi_close_handle_scope(env, scope);
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_SECOND] = { 0 };
    napi_create_string_utf8(env, postDialStr.c_str(), postDialStr.length(), &callbackValues[ARRAY_INDEX_FIRST]);
    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, scope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, scope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdateImsCallModeChange(const CallMediaModeInfo &imsCallModeInfo)
{
    if (imsCallModeCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("thisVar is nullptr!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(imsCallModeCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ImsCallModeInfoWorker *imsCallModeInfoWorker = std::make_unique<ImsCallModeInfoWorker>().release();
    if (imsCallModeInfoWorker == nullptr) {
        TELEPHONY_LOGE("imsCallModeInfoWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    imsCallModeInfoWorker->callModeInfo = imsCallModeInfo;
    imsCallModeInfoWorker->callback = imsCallModeCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete imsCallModeInfoWorker;
        imsCallModeInfoWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)imsCallModeInfoWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("UpdateImsCallModeChange uv_queue_work_with_qos");
        }, ReportCallMediaModeInfoWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete imsCallModeInfoWorker;
        imsCallModeInfoWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportCallMediaModeInfoWork(uv_work_t *work, int32_t status)
{
    ImsCallModeInfoWorker *dataWorkerData = (ImsCallModeInfoWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportCallMediaModeInfo(dataWorkerData->callModeInfo, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportPostDialDelay result = %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportCallMediaModeInfo(
    CallMediaModeInfo &imsCallModeInfo, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope CallMediaModeInfoScope = nullptr;
    napi_open_handle_scope(env, &CallMediaModeInfoScope);
    if (CallMediaModeInfoScope == nullptr) {
        TELEPHONY_LOGE("CallMediaModeInfoScope is nullptr");
        napi_close_handle_scope(env, CallMediaModeInfoScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_SECOND] = { 0 };

    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "callId", imsCallModeInfo.callId);
    NapiCallManagerUtils::SetPropertyBoolean(
        env, callbackValues[ARRAY_INDEX_FIRST], "isRequestInfo", imsCallModeInfo.isRequestInfo);
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "result", imsCallModeInfo.result);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "imsCallMode", static_cast<int32_t>(imsCallModeInfo.callMode));

    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, CallMediaModeInfoScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, CallMediaModeInfoScope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::CallSessionEventChange(const CallSessionEvent &callSessionEvent)
{
    if (callSessionEventCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("thisVar is nullptr!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(callSessionEventCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallSessionEventWorker *callSessionEventWorker = std::make_unique<CallSessionEventWorker>().release();
    if (callSessionEventWorker == nullptr) {
        TELEPHONY_LOGE("callSessionEventWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callSessionEventWorker->sessionEvent = callSessionEvent;
    callSessionEventWorker->callback = callSessionEventCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callSessionEventWorker;
        callSessionEventWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callSessionEventWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("CallSessionEventChange uv_queue_work_with_qos");
        }, ReportCallSessionEventWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete callSessionEventWorker;
        callSessionEventWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportCallSessionEventWork(uv_work_t *work, int32_t status)
{
    CallSessionEventWorker *dataWorkerData = (CallSessionEventWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportCallSessionEvent(dataWorkerData->sessionEvent, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportPostDialDelay result = %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportCallSessionEvent(
    CallSessionEvent &sessionEvent, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope CallSessionEventScope = nullptr;
    napi_open_handle_scope(env, &CallSessionEventScope);
    if (CallSessionEventScope == nullptr) {
        TELEPHONY_LOGE("CallMediaModeInfoScope is nullptr");
        napi_close_handle_scope(env, CallSessionEventScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_SECOND] = { 0 };

    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(env, callbackValues[ARRAY_INDEX_FIRST], "callId", sessionEvent.callId);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "eventId", static_cast<int32_t>(sessionEvent.eventId));

    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, CallSessionEventScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, CallSessionEventScope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::PeerDimensionsChange(const PeerDimensionsDetail &peerDimensionsDetail)
{
    if (peerDimensionsCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("thisVar is nullptr!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(peerDimensionsCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    PeerDimensionsWorker *peerDimensionsWorker = std::make_unique<PeerDimensionsWorker>().release();
    if (peerDimensionsWorker == nullptr) {
        TELEPHONY_LOGE("peerDimensionsWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    peerDimensionsWorker->peerDimensionsDetail = peerDimensionsDetail;
    peerDimensionsWorker->callback = peerDimensionsCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete peerDimensionsWorker;
        peerDimensionsWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)peerDimensionsWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("PeerDimensionsChange uv_queue_work_with_qos");
        }, ReportPeerDimensionsWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete peerDimensionsWorker;
        peerDimensionsWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportPeerDimensionsWork(uv_work_t *work, int32_t status)
{
    PeerDimensionsWorker *dataWorkerData = (PeerDimensionsWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportPeerDimensions(dataWorkerData->peerDimensionsDetail, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportPostDialDelay result = %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportPeerDimensions(
    PeerDimensionsDetail &peerDimensionsDetail, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope PeerDimensionsDetailScope = nullptr;
    napi_open_handle_scope(env, &PeerDimensionsDetailScope);
    if (PeerDimensionsDetailScope == nullptr) {
        TELEPHONY_LOGE("CallMediaModeInfoScope is nullptr");
        napi_close_handle_scope(env, PeerDimensionsDetailScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_SECOND] = { 0 };

    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "callId", peerDimensionsDetail.callId);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "width", peerDimensionsDetail.width);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "height", peerDimensionsDetail.height);

    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, PeerDimensionsDetailScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, PeerDimensionsDetailScope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::CallDataUsageChange(const int64_t dataUsage)
{
    if (callDataUsageCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("thisVar is nullptr!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(callDataUsageCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallDataUsageWorker *callDataUsageWorker = std::make_unique<CallDataUsageWorker>().release();
    if (callDataUsageWorker == nullptr) {
        TELEPHONY_LOGE("callDataUsageWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    callDataUsageWorker->callDataUsage = dataUsage;
    callDataUsageWorker->callback = callDataUsageCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete callDataUsageWorker;
        callDataUsageWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)callDataUsageWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("CallDataUsageChange uv_queue_work_with_qos");
        }, ReportCallDataUsageWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete callDataUsageWorker;
        callDataUsageWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportCallDataUsageWork(uv_work_t *work, int32_t status)
{
    CallDataUsageWorker *dataWorkerData = (CallDataUsageWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportCallDataUsage(dataWorkerData->callDataUsage, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportPostDialDelay result = %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportCallDataUsage(int64_t dataUsage, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope CallDataUsageScope = nullptr;
    napi_open_handle_scope(env, &CallDataUsageScope);
    if (CallDataUsageScope == nullptr) {
        TELEPHONY_LOGE("CallMediaModeInfoScope is nullptr");
        napi_close_handle_scope(env, CallDataUsageScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_SECOND] = { 0 };
    napi_create_int64(env, dataUsage, &callbackValues[ARRAY_INDEX_FIRST]);

    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, CallDataUsageScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, CallDataUsageScope);
    return TELEPHONY_SUCCESS;
}

int32_t NapiCallAbilityCallback::UpdateCameraCapabilities(const CameraCapabilities &cameraCapabilities)
{
    if (cameraCapabilitiesCallback_.thisVar == nullptr) {
        TELEPHONY_LOGE("thisVar is nullptr!");
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    uv_loop_s *loop = nullptr;
#if defined(NAPI_VERSION) && NAPI_VERSION >= 2
    napi_get_uv_event_loop(cameraCapabilitiesCallback_.env, &loop);
#endif
    if (loop == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CameraCapbilitiesWorker *cameraCapbilitiesWorker = std::make_unique<CameraCapbilitiesWorker>().release();
    if (cameraCapbilitiesWorker == nullptr) {
        TELEPHONY_LOGE("cameraCapbilitiesWorker is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    cameraCapbilitiesWorker->cameraCapabilities = cameraCapabilities;
    cameraCapbilitiesWorker->callback = cameraCapabilitiesCallback_;
    uv_work_t *work = std::make_unique<uv_work_t>().release();
    if (work == nullptr) {
        delete cameraCapbilitiesWorker;
        cameraCapbilitiesWorker = nullptr;
        TELEPHONY_LOGE("work is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    work->data = (void *)cameraCapbilitiesWorker;
    int32_t errCode = uv_queue_work_with_qos(
        loop, work, [](uv_work_t *work) {
            TELEPHONY_LOGD("UpdateCameraCapabilities uv_queue_work_with_qos");
        }, ReportCameraCapabilitiesInfoWork, uv_qos_default);
    if (errCode != 0) {
        TELEPHONY_LOGE("failed to uv_queue_work_with_qos, errCode: %{public}d", errCode);
        delete cameraCapbilitiesWorker;
        cameraCapbilitiesWorker = nullptr;
        delete work;
        work = nullptr;
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void NapiCallAbilityCallback::ReportCameraCapabilitiesInfoWork(uv_work_t *work, int32_t status)
{
    CameraCapbilitiesWorker *dataWorkerData = (CameraCapbilitiesWorker *)work->data;
    if (dataWorkerData == nullptr) {
        TELEPHONY_LOGE("dataWorkerData is nullptr!");
        return;
    }
    int32_t ret = ReportCameraCapabilitiesInfo(dataWorkerData->cameraCapabilities, dataWorkerData->callback);
    TELEPHONY_LOGI("ReportPostDialDelay result = %{public}d", ret);
    delete dataWorkerData;
    dataWorkerData = nullptr;
    delete work;
    work = nullptr;
}

int32_t NapiCallAbilityCallback::ReportCameraCapabilitiesInfo(
    CameraCapabilities &cameraCapabilities, EventCallback eventCallback)
{
    napi_env env = eventCallback.env;
    napi_handle_scope cameraCapabilitiesScope = nullptr;
    napi_open_handle_scope(env, &cameraCapabilitiesScope);
    if (cameraCapabilitiesScope == nullptr) {
        TELEPHONY_LOGE("CallMediaModeInfoScope is nullptr");
        napi_close_handle_scope(env, cameraCapabilitiesScope);
        return TELEPHONY_ERROR;
    }
    napi_value callbackFunc = nullptr;
    napi_value callbackValues[ARRAY_INDEX_SECOND] = { 0 };

    napi_create_object(env, &callbackValues[ARRAY_INDEX_FIRST]);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "callId", cameraCapabilities.callId);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "width", cameraCapabilities.width);
    NapiCallManagerUtils::SetPropertyInt32(
        env, callbackValues[ARRAY_INDEX_FIRST], "height", cameraCapabilities.height);

    napi_get_reference_value(env, eventCallback.callbackRef, &callbackFunc);
    if (callbackFunc == nullptr) {
        TELEPHONY_LOGE("callbackFunc is null!");
        napi_close_handle_scope(env, cameraCapabilitiesScope);
        return CALL_ERR_CALLBACK_NOT_EXIST;
    }
    napi_value thisVar = nullptr;
    napi_get_reference_value(env, eventCallback.thisVar, &thisVar);
    napi_value callbackResult = nullptr;
    napi_call_function(env, thisVar, callbackFunc, DATA_LENGTH_ONE, callbackValues, &callbackResult);
    napi_close_handle_scope(env, cameraCapabilitiesScope);
    return TELEPHONY_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS
