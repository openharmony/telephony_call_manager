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

#include "call_status_manager.h"

#include <securec.h>

#include "audio_control_manager.h"
#include "bluetooth_call_service.h"
#include "call_control_manager.h"
#include "call_manager_errors.h"
#include "call_manager_hisysevent.h"
#include "core_service_client.h"
#include "cs_call.h"
#include "datashare_predicates.h"
#include "hitrace_meter.h"
#include "ims_call.h"
#include "ott_call.h"
#include "report_call_info_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
constexpr int32_t INIT_INDEX = 0;
CallStatusManager::CallStatusManager()
{
    (void)memset_s(&callReportInfo_, sizeof(CallDetailInfo), 0, sizeof(CallDetailInfo));
    for (int32_t i = 0; i < SLOT_NUM; i++) {
        (void)memset_s(&callDetailsInfo_[i], sizeof(CallDetailsInfo), 0, sizeof(CallDetailsInfo));
    }
}

CallStatusManager::~CallStatusManager()
{
    UnInit();
}

int32_t CallStatusManager::Init()
{
    for (int32_t i = 0; i < SLOT_NUM; i++) {
        callDetailsInfo_[i].callVec.clear();
    }
    mEventIdTransferMap_.clear();
    mOttEventIdTransferMap_.clear();
    InitCallBaseEvent();
    CallIncomingFilterManagerPtr_ = (std::make_unique<CallIncomingFilterManager>()).release();
    return TELEPHONY_SUCCESS;
}

void CallStatusManager::InitCallBaseEvent()
{
    mEventIdTransferMap_[RequestResultEventId::RESULT_DIAL_NO_CARRIER] = CallAbilityEventId::EVENT_DIAL_NO_CARRIER;
    mEventIdTransferMap_[RequestResultEventId::RESULT_HOLD_SEND_FAILED] = CallAbilityEventId::EVENT_HOLD_CALL_FAILED;
    mEventIdTransferMap_[RequestResultEventId::RESULT_SWAP_SEND_FAILED] = CallAbilityEventId::EVENT_SWAP_CALL_FAILED;
    mOttEventIdTransferMap_[OttCallEventId::OTT_CALL_EVENT_FUNCTION_UNSUPPORTED] =
        CallAbilityEventId::EVENT_OTT_FUNCTION_UNSUPPORTED;
    mEventIdTransferMap_[RequestResultEventId::RESULT_COMBINE_SEND_FAILED] =
        CallAbilityEventId::EVENT_COMBINE_CALL_FAILED;
    mEventIdTransferMap_[RequestResultEventId::RESULT_SPLIT_SEND_FAILED] =
        CallAbilityEventId::EVENT_SPLIT_CALL_FAILED;
}

int32_t CallStatusManager::UnInit()
{
    for (int32_t i = 0; i < SLOT_NUM; i++) {
        callDetailsInfo_[i].callVec.clear();
    }
    mEventIdTransferMap_.clear();
    mOttEventIdTransferMap_.clear();
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::HandleCallReportInfo(const CallDetailInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    callReportInfo_ = info;
    switch (info.state) {
        case TelCallState::CALL_STATUS_ACTIVE:
            ret = ActiveHandle(info);
            break;
        case TelCallState::CALL_STATUS_HOLDING:
            ret = HoldingHandle(info);
            break;
        case TelCallState::CALL_STATUS_DIALING: {
            ret = DialingHandle(info);
            FinishAsyncTrace(HITRACE_TAG_OHOS, "DialCall", getpid());
            DelayedSingleton<CallManagerHisysevent>::GetInstance()->JudgingDialTimeOut(
                info.accountId, static_cast<int32_t>(info.callType), static_cast<int32_t>(info.callMode));
            break;
        }
        case TelCallState::CALL_STATUS_ALERTING:
            ret = AlertHandle(info);
            break;
        case TelCallState::CALL_STATUS_INCOMING: {
            ret = IncomingHandle(info);
            FinishAsyncTrace(HITRACE_TAG_OHOS, "InComingCall", getpid());
            DelayedSingleton<CallManagerHisysevent>::GetInstance()->JudgingIncomingTimeOut(
                info.accountId, static_cast<int32_t>(info.callType), static_cast<int32_t>(info.callMode));
            break;
        }
        case TelCallState::CALL_STATUS_WAITING:
            ret = WaitingHandle(info);
            break;
        case TelCallState::CALL_STATUS_DISCONNECTED:
            ret = DisconnectedHandle(info);
            break;
        case TelCallState::CALL_STATUS_DISCONNECTING:
            ret = DisconnectingHandle(info);
            break;
        default:
            TELEPHONY_LOGE("Invalid call state!");
            break;
    }
    TELEPHONY_LOGI("Entry CallStatusManager HandleCallReportInfo");
    DelayedSingleton<BluetoothCallService>::GetInstance()->GetCallState();
    return ret;
}

// handle call state changes, incoming call, outgoing call.
int32_t CallStatusManager::HandleCallsReportInfo(const CallDetailsInfo &info)
{
    bool flag = false;
    TELEPHONY_LOGI("call list size:%{public}zu,slotId:%{public}d", info.callVec.size(), info.slotId);
    int32_t curSlotId = info.slotId;
    for (auto &it : info.callVec) {
        for (const auto &it1 : callDetailsInfo_[curSlotId].callVec) {
            if (it.index == it1.index) {
                // call state changes
                if (it.state != it1.state || it.mpty != it1.mpty || it.callType != it1.callType) {
                    TELEPHONY_LOGI("handle updated call state:%{public}d", it.state);
                    HandleCallReportInfo(it);
                }
                flag = true;
                break;
            }
        }
        // incoming/outgoing call handle
        if (!flag || callDetailsInfo_[curSlotId].callVec.empty()) {
            TELEPHONY_LOGI("handle new call state:%{public}d", it.state);
            HandleCallReportInfo(it);
        }
        flag = false;
    }
    // disconnected calls handle
    for (auto &it2 : callDetailsInfo_[curSlotId].callVec) {
        for (const auto &it3 : info.callVec) {
            if (it2.index == it3.index) {
                TELEPHONY_LOGI("state:%{public}d", it2.state);
                flag = true;
                break;
            }
        }
        if (!flag) {
            it2.state = TelCallState::CALL_STATUS_DISCONNECTED;
            HandleCallReportInfo(it2);
        }
        flag = false;
    }
    callDetailsInfo_[curSlotId].callVec.clear();
    callDetailsInfo_[curSlotId] = info;
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::HandleDisconnectedCause(const DisconnectedDetails &details)
{
    bool ret = DelayedSingleton<CallControlManager>::GetInstance()->NotifyCallDestroyed(details);
    if (!ret) {
        TELEPHONY_LOGI("NotifyCallDestroyed failed!");
        return CALL_ERR_PHONE_CALLSTATE_NOTIFY_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::HandleEventResultReportInfo(const CellularCallEventInfo &info)
{
    if (info.eventType != CellularCallEventType::EVENT_REQUEST_RESULT_TYPE) {
        TELEPHONY_LOGE("unexpected type event occurs, eventId:%{public}d", info.eventId);
        return CALL_ERR_PHONE_TYPE_UNEXPECTED;
    }
    TELEPHONY_LOGI("recv one Event, eventId:%{public}d", info.eventId);
    sptr<CallBase> call = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_DIALING);
    if (call != nullptr) {
        int32_t ret = DealFailDial(call);
        TELEPHONY_LOGI("DealFailDial ret:%{public}d", ret);
    }
    CallEventInfo eventInfo;
    (void)memset_s(&eventInfo, sizeof(CallEventInfo), 0, sizeof(CallEventInfo));
    if (mEventIdTransferMap_.find(info.eventId) != mEventIdTransferMap_.end()) {
        eventInfo.eventId = mEventIdTransferMap_[info.eventId];
        DialParaInfo dialInfo;
        if (eventInfo.eventId == CallAbilityEventId::EVENT_DIAL_NO_CARRIER) {
            DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(dialInfo);
            if (dialInfo.number.length() > static_cast<size_t>(kMaxNumberLen)) {
                TELEPHONY_LOGE("Number out of limit!");
                return CALL_ERR_NUMBER_OUT_OF_RANGE;
            }
            if (memcpy_s(eventInfo.phoneNum, kMaxNumberLen, dialInfo.number.c_str(), dialInfo.number.length()) != EOK) {
                TELEPHONY_LOGE("memcpy_s failed!");
                return TELEPHONY_ERR_MEMCPY_FAIL;
            }
        } else if (eventInfo.eventId == CallAbilityEventId::EVENT_COMBINE_CALL_FAILED) {
            sptr<CallBase> activeCall = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_ACTIVE);
            if (activeCall != nullptr) {
                activeCall->HandleCombineConferenceFailEvent();
            }
        }
        DelayedSingleton<CallControlManager>::GetInstance()->NotifyCallEventUpdated(eventInfo);
    } else {
        TELEPHONY_LOGW("unknown type Event, eventid %{public}d", info.eventId);
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::HandleOttEventReportInfo(const OttCallEventInfo &info)
{
    TELEPHONY_LOGI("recv one Event, eventId:%{public}d", info.ottCallEventId);
    CallEventInfo eventInfo;
    (void)memset_s(&eventInfo, sizeof(CallEventInfo), 0, sizeof(CallEventInfo));
    if (mOttEventIdTransferMap_.find(info.ottCallEventId) != mOttEventIdTransferMap_.end()) {
        eventInfo.eventId = mOttEventIdTransferMap_[info.ottCallEventId];
        if (strlen(info.bundleName) > static_cast<size_t>(kMaxNumberLen)) {
            TELEPHONY_LOGE("Number out of limit!");
            return CALL_ERR_NUMBER_OUT_OF_RANGE;
        }
        if (memcpy_s(eventInfo.bundleName, kMaxNumberLen, info.bundleName, strlen(info.bundleName)) != EOK) {
            TELEPHONY_LOGE("memcpy_s failed!");
            return TELEPHONY_ERR_MEMCPY_FAIL;
        }
        DelayedSingleton<CallControlManager>::GetInstance()->NotifyCallEventUpdated(eventInfo);
    } else {
        TELEPHONY_LOGW("unknown type Event, eventid %{public}d", info.ottCallEventId);
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::IncomingHandle(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndex(info.index);
    if (call != nullptr && call->GetCallType() != info.callType) {
        call = RefreshCallIfNecessary(call, info);
        return TELEPHONY_SUCCESS;
    }
    int32_t ret = IncomingHandlePolicy(info);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("IncomingHandlePolicy failed!");
        if (info.state == TelCallState::CALL_STATUS_INCOMING) {
            CallManagerHisysevent::WriteIncomingCallFaultEvent(info.accountId, static_cast<int32_t>(info.callType),
                static_cast<int32_t>(info.callMode), ret, "IncomingHandlePolicy failed");
        }
        return ret;
    }
    if (info.callType == CallType::TYPE_CS || info.callType == CallType::TYPE_IMS) {
        ret = IncomingFilterPolicy(info);
        if (ret != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGE("IncomingFilterPolicy failed!");
            return ret;
        }
    }
    call = CreateNewCall(info, CallDirection::CALL_DIRECTION_IN);
    if (call == nullptr) {
        TELEPHONY_LOGE("CreateNewCall failed!");
        return CALL_ERR_CALL_OBJECT_IS_NULL;
    }

    // allow list filtering
    // Get the contact data from the database
    ContactInfo contactInfo = {
        .name = "",
        .number = "",
        .isContacterExists = false,
        .ringtonePath = "",
        .isSendToVoicemail = false,
        .isEcc = false,
        .isVoiceMail = false,
    };
    QueryCallerInfo(contactInfo, std::string(info.phoneNum));
    call->SetCallerInfo(contactInfo);

    DelayedSingleton<CallControlManager>::GetInstance()->NotifyNewCallCreated(call);
    ret = UpdateCallState(call, info.state);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed!");
        return ret;
    }
    ret = FilterResultsDispose(call);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("FilterResultsDispose failed!");
    }
    return ret;
}

void CallStatusManager::QueryCallerInfo(ContactInfo &contactInfo, std::string phoneNum)
{
    TELEPHONY_LOGI("Entry CallStatusManager QueryCallerInfo");
    std::shared_ptr<CallDataBaseHelper> callDataPtr = DelayedSingleton<CallDataBaseHelper>::GetInstance();
    if (callDataPtr == nullptr) {
        TELEPHONY_LOGE("callDataPtr is nullptr!");
        return;
    }
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(DETAIL_INFO, phoneNum);
    predicates.And();
    predicates.EqualTo(CONTENT_TYPE, PHONE);
    bool ret = callDataPtr->Query(contactInfo, predicates);
    if (!ret) {
        TELEPHONY_LOGE("Query contact database fail!");
    }
}

int32_t CallStatusManager::IncomingFilterPolicy(const CallDetailInfo &info)
{
    if (CallIncomingFilterManagerPtr_ == nullptr) {
        TELEPHONY_LOGE("CallIncomingFilterManagerPtr_ is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return CallIncomingFilterManagerPtr_->DoIncomingFilter(info);
}

void CallStatusManager::CallFilterCompleteResult(const CallDetailInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    sptr<CallBase> call = CreateNewCall(info, CallDirection::CALL_DIRECTION_IN);
    if (call == nullptr) {
        TELEPHONY_LOGE("CreateNewCall failed!");
        return;
    }
#ifdef ABILITY_DATABASE_SUPPORT
    // allow list filtering
    // Get the contact data from the database
    GetCallerInfoDate(ContactInfo);
    SetCallerInfo(contactInfo);
#endif
    DelayedSingleton<CallControlManager>::GetInstance()->NotifyNewCallCreated(call);
    ret = UpdateCallState(call, info.state);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed!");
        return;
    }
    ret = FilterResultsDispose(call);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("FilterResultsDispose failed!");
        return;
    }
}

int32_t CallStatusManager::UpdateDialingCallInfo(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndex(info.index);
    if (call != nullptr) {
        call = RefreshCallIfNecessary(call, info);
        return TELEPHONY_SUCCESS;
    }
    call = GetOneCallObjectByIndex(INIT_INDEX);
    if (call == nullptr) {
        TELEPHONY_LOGE("call is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }

    std::string oriNum = call->GetAccountNumber();
    call = RefreshCallIfNecessary(call, info);
    call->SetCallIndex(info.index);
    call->SetBundleName(info.bundleName);
    call->SetSlotId(info.accountId);
    call->SetTelCallState(info.state);
    call->SetVideoStateType(info.callMode);
    call->SetCallType(info.callType);
    call->SetAccountNumber(oriNum);
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::DialingHandle(const CallDetailInfo &info)
{
    TELEPHONY_LOGI("handle dialing state");
    if (info.index > 0) {
        TELEPHONY_LOGI("need update call info");
        return UpdateDialingCallInfo(info);
    }
    sptr<CallBase> call = CreateNewCall(info, CallDirection::CALL_DIRECTION_OUT);
    if (call == nullptr) {
        TELEPHONY_LOGE("CreateNewCall failed!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    int32_t ret = call->DialingProcess();
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    DelayedSingleton<CallControlManager>::GetInstance()->NotifyNewCallCreated(call);
    ret = UpdateCallState(call, TelCallState::CALL_STATUS_DIALING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusManager::ActiveHandle(const CallDetailInfo &info)
{
    TELEPHONY_LOGI("handle active state");
    std::string tmpStr(info.phoneNum);
    sptr<CallBase> firstCall = GetOneCallObjectByIndex(info.index);
    sptr<CallBase> secondCall = GetOneCallObject(tmpStr);
    if (firstCall != secondCall || firstCall == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    sptr<CallBase> call = firstCall;
    call = RefreshCallIfNecessary(call, info);
    // call state change active, need to judge if launching a conference
    if (info.mpty == 1) {
        int32_t mainCallId = ERR_ID;
        call->LaunchConference();
        call->GetMainCallId(mainCallId);
        sptr<CallBase> mainCall = GetOneCallObject(mainCallId);
        if (mainCall != nullptr) {
            mainCall->SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_ACTIVE);
        }
    } else if (call->ExitConference() == TELEPHONY_SUCCESS) {
        TELEPHONY_LOGI("SubCallSeparateFromConference success!");
    } else {
        TELEPHONY_LOGI("SubCallSeparateFromConference fail!");
    }
    int32_t ret = UpdateCallState(call, TelCallState::CALL_STATUS_ACTIVE);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
        return ret;
    }
#ifdef AUDIO_SUPPORT
    ToSpeakerPhone(call);
    DelayedSingleton<AudioControlManager>::GetInstance()->SetVolumeAudible();
#endif
    TELEPHONY_LOGI("handle active state success");
    return ret;
}

int32_t CallStatusManager::HoldingHandle(const CallDetailInfo &info)
{
    TELEPHONY_LOGI("handle holding state");
    std::string tmpStr(info.phoneNum);
    sptr<CallBase> firstCall = GetOneCallObjectByIndex(info.index);
    sptr<CallBase> secondCall = GetOneCallObject(tmpStr);
    if (firstCall != secondCall || firstCall == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    sptr<CallBase> call = firstCall;
    // if the call is in a conference, it will exit, otherwise just set it holding
    call = RefreshCallIfNecessary(call, info);
    int32_t ret = call->HoldConference();
    if (ret == TELEPHONY_SUCCESS) {
        TELEPHONY_LOGI("HoldConference success");
    }
    ret = UpdateCallState(call, TelCallState::CALL_STATUS_HOLDING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
    }
    int32_t dsdsMode = DSDS_MODE_V2;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().GetDsdsMode(dsdsMode);
    TELEPHONY_LOGE("HoldingHandle dsdsMode:%{public}d", dsdsMode);
    if (dsdsMode == static_cast<int32_t>(DsdsMode::DSDS_MODE_V5) ||
        dsdsMode == static_cast<int32_t>(DsdsMode::DSDS_MODE_TDM)) {
        int32_t activeCallNum = GetCallNum(TelCallState::CALL_STATUS_ACTIVE);
        AutoAnswerForDsda(activeCallNum);
    }
    return ret;
}

int32_t CallStatusManager::WaitingHandle(const CallDetailInfo &info)
{
    return IncomingHandle(info);
}

int32_t CallStatusManager::AlertHandle(const CallDetailInfo &info)
{
    TELEPHONY_LOGI("handle alerting state");
    std::string tmpStr(info.phoneNum);
    sptr<CallBase> firstCall = GetOneCallObjectByIndex(info.index);
    sptr<CallBase> secondCall = GetOneCallObject(tmpStr);
    if (firstCall != secondCall || firstCall == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    sptr<CallBase> call = firstCall;
    call = RefreshCallIfNecessary(call, info);
    int32_t ret = UpdateCallState(call, TelCallState::CALL_STATUS_ALERTING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
        return ret;
    }
#ifdef AUDIO_SUPPORT
    ToSpeakerPhone(call);
    TurnOffMute(call);
    DelayedSingleton<AudioControlManager>::GetInstance()->SetVolumeAudible();
#endif
    return ret;
}

int32_t CallStatusManager::DisconnectingHandle(const CallDetailInfo &info)
{
    TELEPHONY_LOGI("handle disconnecting state");
    std::string tmpStr(info.phoneNum);
    sptr<CallBase> firstCall = GetOneCallObjectByIndex(info.index);
    sptr<CallBase> secondCall = GetOneCallObject(tmpStr);
    if (firstCall != secondCall || firstCall == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    sptr<CallBase> call = firstCall;
    call = RefreshCallIfNecessary(call, info);
    int32_t ret = UpdateCallState(call, TelCallState::CALL_STATUS_DISCONNECTING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusManager::DisconnectedHandle(const CallDetailInfo &info)
{
    TELEPHONY_LOGI("handle disconnected state");
    std::string tmpStr(info.phoneNum);
    sptr<CallBase> firstCall = GetOneCallObjectByIndex(info.index);
    sptr<CallBase> secondCall = GetOneCallObject(tmpStr);
    if (firstCall != secondCall || firstCall == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    sptr<CallBase> call = firstCall;
    call = RefreshCallIfNecessary(call, info);
    bool canUnHold = false;
    std::vector<std::u16string> callIdList;
    call->GetSubCallIdList(callIdList);
    CallRunningState previousState = call->GetCallRunningState();
    int32_t ret = call->ExitConference();
    if (ret == TELEPHONY_SUCCESS) {
        TELEPHONY_LOGI("SubCallSeparateFromConference success");
    }
    ret = UpdateCallState(call, TelCallState::CALL_STATUS_DISCONNECTED);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
        return ret;
    }
    int32_t activeCallNum = GetCallNum(TelCallState::CALL_STATUS_ACTIVE);
    int32_t waitingCallNum = GetCallNum(TelCallState::CALL_STATUS_WAITING);
    if ((callIdList.size() == 0 || callIdList.size() == 1) && activeCallNum == 0 && waitingCallNum == 0) {
        canUnHold = true;
    }
    sptr<CallBase> holdCall = CallObjectManager::GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_HOLD);
    if (previousState != CallRunningState::CALL_RUNNING_STATE_HOLD &&
        previousState != CallRunningState::CALL_RUNNING_STATE_ACTIVE) {
        if (holdCall != nullptr && canUnHold) {
            TELEPHONY_LOGI("release call and recover the held call");
            holdCall->UnHoldCall();
        }
    }
    DeleteOneCallObject(call->GetCallID());
    int32_t dsdsMode = DSDS_MODE_V2;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().GetDsdsMode(dsdsMode);
    TELEPHONY_LOGE("DisconnectedHandle dsdsMode:%{public}d", dsdsMode);
    if (dsdsMode == DSDS_MODE_V3) {
        AutoAnswer(activeCallNum, waitingCallNum);
    } else if (dsdsMode == static_cast<int32_t>(DsdsMode::DSDS_MODE_V5) ||
               dsdsMode == static_cast<int32_t>(DsdsMode::DSDS_MODE_TDM)) {
        AutoAnswerForDsda(activeCallNum);
    }
    return ret;
}

void CallStatusManager::AutoAnswerForDsda(int32_t activeCallNum)
{
    int32_t dialingCallNum = GetCallNum(TelCallState::CALL_STATUS_DIALING);
    int32_t alertingCallNum = GetCallNum(TelCallState::CALL_STATUS_ALERTING);
    sptr<CallBase> ringCall = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_RINGING);
    TELEPHONY_LOGE("ringCall is not null  =:%{public}d", ringCall != nullptr);
    if (ringCall != nullptr && dialingCallNum == 0 && alertingCallNum == 0 && activeCallNum == 0 &&
        ringCall->GetAutoAnswerState()) {
        int32_t videoState = static_cast<int32_t>(ringCall->GetVideoStateType());
        int ret = ringCall->AnswerCall(videoState);
        TELEPHONY_LOGI("ret = %{public}d", ret);
        ringCall->SetAutoAnswerState(false);
    }
}

void CallStatusManager::AutoAnswer(int32_t activeCallNum, int32_t waitingCallNum)
{
    bool flag = false;
    int32_t holdingCallNum = GetCallNum(TelCallState::CALL_STATUS_HOLDING);
    int32_t dialingCallNum = GetCallNum(TelCallState::CALL_STATUS_DIALING);
    int32_t alertingCallNum = GetCallNum(TelCallState::CALL_STATUS_ALERTING);
    if (activeCallNum == 0 && waitingCallNum == 0 && holdingCallNum == 0 && dialingCallNum == 0 &&
        alertingCallNum == 0) {
        std::list<int32_t> ringCallIdList;
        GetCarrierCallList(ringCallIdList);
        for (int32_t ringingCallId : ringCallIdList) {
            sptr<CallBase> ringingCall = GetOneCallObject(ringingCallId);
            CallRunningState ringingCallState = ringingCall->GetCallRunningState();
            if ((ringingCallState == CallRunningState::CALL_RUNNING_STATE_RINGING &&
                    (ringingCall->GetAutoAnswerState()))) {
                ringingCall->SetAutoAnswerState(flag);
                int32_t videoState = static_cast<int32_t>(ringingCall->GetVideoStateType());
                int ret = ringingCall->AnswerCall(videoState);
                TELEPHONY_LOGI("ret = %{public}d", ret);
                break;
            }
        }
    }
}

int32_t CallStatusManager::UpdateCallState(sptr<CallBase> &call, TelCallState nextState)
{
    TELEPHONY_LOGI("UpdateCallState start");
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    TelCallState priorState = call->GetTelCallState();
    TELEPHONY_LOGI("priorState:%{public}d, nextState:%{public}d", priorState, nextState);
    if (priorState == TelCallState::CALL_STATUS_INCOMING && nextState == TelCallState::CALL_STATUS_ACTIVE) {
        DelayedSingleton<CallManagerHisysevent>::GetInstance()->JudgingAnswerTimeOut(
            call->GetSlotId(), call->GetCallID(), static_cast<int32_t>(call->GetVideoStateType()));
    }
    // need DTMF judge
    int32_t ret = call->SetTelCallState(nextState);
    if (ret != TELEPHONY_SUCCESS && ret != CALL_ERR_NOT_NEW_STATE) {
        TELEPHONY_LOGE("SetTelCallState failed");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    // notify state changed
    if (!DelayedSingleton<CallControlManager>::GetInstance()->NotifyCallStateUpdated(call, priorState, nextState)) {
        TELEPHONY_LOGE(
            "NotifyCallStateUpdated failed! priorState:%{public}d,nextState:%{public}d", priorState, nextState);
        if (nextState == TelCallState::CALL_STATUS_INCOMING) {
            CallManagerHisysevent::WriteIncomingCallFaultEvent(call->GetSlotId(),
                static_cast<int32_t>(call->GetCallType()), static_cast<int32_t>(call->GetVideoStateType()), ret,
                "NotifyCallStateUpdated failed");
        }
        return CALL_ERR_PHONE_CALLSTATE_NOTIFY_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

sptr<CallBase> CallStatusManager::RefreshCallIfNecessary(const sptr<CallBase> &call, const CallDetailInfo &info)
{
    TELEPHONY_LOGI("RefreshCallIfNecessary");
    if (call->GetCallType() == info.callType) {
        TELEPHONY_LOGI("RefreshCallIfNecessary not need Refresh");
        return call;
    }
    TelCallState priorState = call->GetTelCallState();
    CallAttributeInfo attrInfo;
    (void)memset_s(&attrInfo, sizeof(CallAttributeInfo), 0, sizeof(CallAttributeInfo));
    call->GetCallAttributeBaseInfo(attrInfo);
    sptr<CallBase> newCall = CreateNewCall(info, attrInfo.callDirection);
    if (newCall == nullptr) {
        TELEPHONY_LOGE("RefreshCallIfNecessary createCallFail");
        return call;
    }
    newCall->SetCallRunningState(call->GetCallRunningState());
    newCall->SetTelConferenceState(call->GetTelConferenceState());
    newCall->SetStartTime(attrInfo.startTime);
    newCall->SetPolicyFlag(PolicyFlag(call->GetPolicyFlag()));
    newCall->SetSpeakerphoneOn(call->IsSpeakerphoneOn());
    newCall->SetCallEndedType(call->GetCallEndedType());
    newCall->SetCallBeginTime(attrInfo.callBeginTime);
    newCall->SetCallEndTime(attrInfo.callEndTime);
    newCall->SetRingBeginTime(attrInfo.ringBeginTime);
    newCall->SetRingEndTime(attrInfo.ringEndTime);
    newCall->SetAnswerType(attrInfo.answerType);
    DeleteOneCallObject(call->GetCallID());
    newCall->SetCallId(call->GetCallID());
    newCall->SetTelCallState(priorState);
    return newCall;
}

int32_t CallStatusManager::ToSpeakerPhone(sptr<CallBase> &call)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (call->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_DIALING) {
        TELEPHONY_LOGI("Call is CALL_STATUS_DIALING");
        return ret;
    }
    if (call->IsSpeakerphoneOn()) {
        AudioDevice device = {
            .deviceType = AudioDeviceType::DEVICE_SPEAKER,
            .address = { 0 },
        };
        DelayedSingleton<AudioControlManager>::GetInstance()->SetAudioDevice(device);
        ret = call->SetSpeakerphoneOn(false);
    }
    return ret;
}

int32_t CallStatusManager::TurnOffMute(sptr<CallBase> &call)
{
    bool enabled = true;
    if (HasEmergencyCall(enabled) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGI("CallStatusManager::TurnOffMute HasEmergencyCall failed.");
    }
    if (call->GetEmergencyState() || enabled) {
        DelayedSingleton<AudioControlManager>::GetInstance()->SetMute(false);
    } else {
        DelayedSingleton<AudioControlManager>::GetInstance()->SetMute(true);
    }
    return TELEPHONY_SUCCESS;
}

sptr<CallBase> CallStatusManager::CreateNewCall(const CallDetailInfo &info, CallDirection dir)
{
    sptr<CallBase> callPtr = nullptr;
    DialParaInfo paraInfo;
    AppExecFwk::PacMap extras;
    extras.Clear();
    PackParaInfo(paraInfo, info, dir, extras);
    switch (info.callType) {
        case CallType::TYPE_CS: {
            if (dir == CallDirection::CALL_DIRECTION_OUT) {
                callPtr = (std::make_unique<CSCall>(paraInfo, extras)).release();
            } else {
                callPtr = (std::make_unique<CSCall>(paraInfo)).release();
            }
            break;
        }
        case CallType::TYPE_IMS: {
            if (dir == CallDirection::CALL_DIRECTION_OUT) {
                callPtr = (std::make_unique<IMSCall>(paraInfo, extras)).release();
            } else {
                callPtr = (std::make_unique<IMSCall>(paraInfo)).release();
            }
            break;
        }
        case CallType::TYPE_OTT: {
            if (dir == CallDirection::CALL_DIRECTION_OUT) {
                callPtr = (std::make_unique<OTTCall>(paraInfo, extras)).release();
            } else {
                callPtr = (std::make_unique<OTTCall>(paraInfo)).release();
            }
            break;
        }
        default:
            return nullptr;
    }
    if (callPtr == nullptr) {
        TELEPHONY_LOGE("CreateNewCall failed!");
        return nullptr;
    }
    AddOneCallObject(callPtr);
    return callPtr;
}

void CallStatusManager::PackParaInfo(
    DialParaInfo &paraInfo, const CallDetailInfo &info, CallDirection dir, AppExecFwk::PacMap &extras)
{
    paraInfo.isEcc = false;
    paraInfo.dialType = DialType::DIAL_CARRIER_TYPE;
    if (dir == CallDirection::CALL_DIRECTION_OUT) {
        DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(paraInfo, extras);
    }
    paraInfo.number = info.phoneNum;
    paraInfo.callId = GetNewCallId();
    paraInfo.index = info.index;
    paraInfo.videoState = VideoStateType::TYPE_VOICE;
    paraInfo.accountId = info.accountId;
    paraInfo.callType = info.callType;
    paraInfo.callState = info.state;
    paraInfo.bundleName = info.bundleName;
}
} // namespace Telephony
} // namespace OHOS
