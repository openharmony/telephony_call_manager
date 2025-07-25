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

#include "call_policy.h"

#include "call_dialog.h"
#include "call_data_base_helper.h"
#include "call_manager_errors.h"
#include "call_number_utils.h"
#include "core_service_client.h"
#include "ims_conference.h"
#include "telephony_log_wrapper.h"
#include "call_control_manager.h"
#include "call_superprivacy_control_manager.h"
#include "call_manager_base.h"
#include "distributed_communication_manager.h"
#include "cellular_call_connection.h"

namespace OHOS {
namespace Telephony {
CallPolicy::CallPolicy() : edmCallPolicy_(std::make_shared<EdmCallPolicy>()) {}

CallPolicy::~CallPolicy() {}

int32_t CallPolicy::DialPolicy(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc)
{
    DialType dialType = (DialType)extras.GetIntValue("dialType");
    if (dialType != DialType::DIAL_CARRIER_TYPE && dialType != DialType::DIAL_VOICE_MAIL_TYPE &&
        dialType != DialType::DIAL_OTT_TYPE && dialType != DialType::DIAL_BLUETOOTH_TYPE) {
        TELEPHONY_LOGE("dial type invalid! dialType=%{public}d", dialType);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    std::string phoneNum = Str16ToStr8(number);
    if (!IsDialingEnable(phoneNum)) {
        TELEPHONY_LOGE("MDM policy is disabled!");
        return TELEPHONY_ERR_POLICY_DISABLED;
    }
    int32_t accountId = extras.GetIntValue("accountId");
    if (dialType == DialType::DIAL_CARRIER_TYPE) {
        if (!DelayedSingleton<CallNumberUtils>::GetInstance()->SelectAccountId(accountId, extras)) {
            extras.PutIntValue("accountId", 0);
            TELEPHONY_LOGE("invalid accountId, select accountId to 0");
        }
    }
    CallType callType = (CallType)extras.GetIntValue("callType");
    if (IsValidCallType(callType) != TELEPHONY_SUCCESS) {
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    DialScene dialScene = (DialScene)extras.GetIntValue("dialScene");
    if ((dialScene != DialScene::CALL_NORMAL && dialScene != DialScene::CALL_PRIVILEGED &&
            dialScene != DialScene::CALL_EMERGENCY)) {
        TELEPHONY_LOGE("invalid dial scene!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    VideoStateType videoState = (VideoStateType)extras.GetIntValue("videoState");
    if (videoState != VideoStateType::TYPE_VOICE && videoState != VideoStateType::TYPE_VIDEO) {
        TELEPHONY_LOGE("invalid video state!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (!isEcc) {
        if (IsVoiceCallValid(videoState) != TELEPHONY_SUCCESS) {
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
        if (HasNewCall() != TELEPHONY_SUCCESS) {
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
        bool hasEccCall = false;
        if (HasEmergencyCall(hasEccCall) == TELEPHONY_ERR_SUCCESS && hasEccCall) {
            TELEPHONY_LOGE("during emergency call, calling is prohibited");
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
    }
    return SuperPrivacyMode(number, extras, isEcc);
}

int32_t CallPolicy::SuperPrivacyMode(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc)
{
    int32_t accountId = extras.GetIntValue("accountId");
    CallType callType = (CallType)extras.GetIntValue("callType");
    int32_t slotId = extras.GetIntValue("accountId");
    if (isEcc) {
        return HasNormalCall(isEcc, slotId, callType);
    }
    DialScene dialScene = (DialScene)extras.GetIntValue("dialScene");
    if (dialScene == DialScene::CALL_EMERGENCY) {
        return HasNormalCall(isEcc, slotId, callType);
    }
    bool currentIsSuperPrivacyMode = DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->
        GetCurrentIsSuperPrivacyMode();
    TELEPHONY_LOGI("call policy currentIsSuperPrivacyMode:%{public}d", currentIsSuperPrivacyMode);
    if (currentIsSuperPrivacyMode) {
        int32_t videoState = extras.GetIntValue("videoState");
        int32_t dialType = extras.GetIntValue("dialType");
        int32_t dialScene = extras.GetIntValue("dialScene");
        int32_t spCallType = extras.GetIntValue("callType");
        DelayedSingleton<CallDialog>::GetInstance()->DialogConnectPrivpacyModeExtension("SUPER_PRIVACY_MODE",
            number, accountId, videoState, dialType, dialScene, spCallType, true);
        return CALL_ERR_DIAL_FAILED;
    }
    return HasNormalCall(isEcc, slotId, callType);
}
int32_t CallPolicy::HasNormalCall(bool isEcc, int32_t slotId, CallType callType)
{
    if (isEcc || callType == CallType::TYPE_SATELLITE || callType == CallType::TYPE_BLUETOOTH) {
        return TELEPHONY_SUCCESS;
    }
    bool hasSimCard = false;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().HasSimCard(slotId, hasSimCard);
    if (!hasSimCard) {
        TELEPHONY_LOGE("Call failed due to no sim card");
        DelayedSingleton<CallDialog>::GetInstance()->DialogConnectExtension("CALL_FAILED_NO_SIM_CARD");
        return TELEPHONY_ERR_NO_SIM_CARD;
    }
    bool isAirplaneModeOn = false;
    int32_t ret = GetAirplaneMode(isAirplaneModeOn);
    if (ret == TELEPHONY_SUCCESS && isAirplaneModeOn) {
        TELEPHONY_LOGE("Call failed due to isAirplaneModeOn is true");
        DelayedSingleton<CallDialog>::GetInstance()->DialogConnectExtension("CALL_FAILED_IN_AIRPLANE_MODE");
        return TELEPHONY_ERR_AIRPLANE_MODE_ON;
    }
    sptr<NetworkState> networkState = nullptr;
    RegServiceState regStatus = RegServiceState::REG_STATE_UNKNOWN;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().GetNetworkState(slotId, networkState);
    if (networkState != nullptr) {
        regStatus = networkState->GetRegStatus();
    }
    if (regStatus != RegServiceState::REG_STATE_IN_SERVICE) {
        TELEPHONY_LOGE("Call failed due to no service");
        DelayedSingleton<CallDialog>::GetInstance()->DialogConnectExtension("CALL_FAILED_NO_SERVICE");
        return TELEPHONY_ERR_NETWORK_NOT_IN_SERVICE;
    }
    ImsRegInfo info;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().GetImsRegStatus(slotId, ImsServiceType::TYPE_VOICE, info);
    bool isImsRegistered = info.imsRegState == ImsRegState::IMS_REGISTERED;
    bool isCTSimCard = false;
    bool isRoaming = networkState->IsRoaming();
    DelayedRefSingleton<CoreServiceClient>::GetInstance().IsCTSimCard(slotId, isCTSimCard);
    if (isCTSimCard && !isRoaming && !isImsRegistered && IsCtSimCardSwitchToChnOrMc(slotId)) {
        TELEPHONY_LOGE("Call failed due to CT card IMS is UNREGISTERED");
        DelayedSingleton<CallDialog>::GetInstance()->DialogConnectExtension("CALL_FAILED_CTCARD_NO_IMS", slotId);
        return CALL_ERR_DIAL_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

bool CallPolicy::IsCtSimCardSwitchToChnOrMc(int32_t slotId)
{
    std::string regPlmn = DelayedRefSingleton<CoreServiceClient>::GetInstance().GetResidentNetworkNumeric(slotId);
    std::u16string hplmn;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().GetSimOperatorNumeric(slotId, hplmn);
    std::string simOperator =
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(hplmn);
    TELEPHONY_LOGI("regPlmn = %{public}s, simOperator = %{public}s", regPlmn.c_str(), simOperator.c_str());
    if (regPlmn.empty() || simOperator.empty() || regPlmn.length() < MCC_LEN || simOperator.length() < MCC_LEN) {
        return false;
    }
    return (regPlmn.substr(0, MCC_LEN).compare(CHN_MCC) == 0 && simOperator.substr(0, MCC_LEN).compare(CHN_MCC) == 0)
        || (regPlmn.substr(0, MCC_LEN).compare(MC_MCC) == 0 && simOperator.substr(0, MCC_LEN).compare(MC_MCC) == 0);
}

int32_t CallPolicy::GetAirplaneMode(bool &isAirplaneModeOn)
{
    std::shared_ptr<CallDataBaseHelper> callDataPtr = DelayedSingleton<CallDataBaseHelper>::GetInstance();
    if (callDataPtr == nullptr) {
        TELEPHONY_LOGE("callDataPtr is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return callDataPtr->GetAirplaneMode(isAirplaneModeOn);
}

int32_t CallPolicy::IsVoiceCallValid(VideoStateType videoState)
{
    if (videoState == VideoStateType::TYPE_VOICE) {
        sptr<CallBase> ringCall = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_RINGING);
        if (ringCall != nullptr && ringCall->GetVideoStateType() == VideoStateType::TYPE_VOICE &&
            ringCall->GetCallType() != CallType::TYPE_VOIP) {
            TELEPHONY_LOGE("already has new call ringing!");
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::IsValidCallType(CallType callType)
{
    if (callType != CallType::TYPE_CS && callType != CallType::TYPE_IMS && callType != CallType::TYPE_OTT &&
        callType != CallType::TYPE_SATELLITE && callType != CallType::TYPE_BLUETOOTH) {
        TELEPHONY_LOGE("invalid call type!");
        return CALL_ERR_UNKNOW_CALL_TYPE;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::CanDialMulityCall(AppExecFwk::PacMap &extras, bool isEcc)
{
    VideoStateType videoState = (VideoStateType)extras.GetIntValue("videoState");
    int32_t slotId = extras.GetIntValue("accountId");
    bool enabled = false;
    DelayedSingleton<CellularCallConnection>::GetInstance()->GetVideoCallWaiting(slotId, enabled);
    if (!enabled) {
        if (videoState == VideoStateType::TYPE_VIDEO && HasCellularCallExist()) {
            TELEPHONY_LOGE("can not dial video call when any call exist!");
            return CALL_ERR_DIAL_IS_BUSY;
        }
        if (!isEcc && videoState == VideoStateType::TYPE_VOICE && HasVideoCall()) {
            TELEPHONY_LOGE("can not dial video call when any call exist!");
            return CALL_ERR_DIAL_IS_BUSY;
        }
    }
    if (DelayedSingleton<DistributedCommunicationManager>::GetInstance()->IsConnected() &&
        DelayedSingleton<DistributedCommunicationManager>::GetInstance()->IsSinkRole() &&
        HasCellularCallExist()) {
        TELEPHONY_LOGE("dc-call sink can not dial call when any call exist!");
        return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
    }
    return TELEPHONY_SUCCESS;
}

bool CallPolicy::IsSupportVideoCall(AppExecFwk::PacMap &extras)
{
    bool isSupportVideoCall = true;
#ifdef ABILITY_CONFIG_SUPPORT
    isSupportVideoCall = GetCarrierConfig(ITEM_VIDEO_CALL);
#endif
    bool vtEnabled = false;
    DelayedSingleton<CallNumberUtils>::GetInstance()->IsCarrierVtConfig(extras.GetIntValue("accountId"), vtEnabled);
    if (!vtEnabled) {
        isSupportVideoCall = false;
        return isSupportVideoCall;
    }
    DialScene dialScene = (DialScene)extras.GetIntValue("dialScene");
    if (dialScene != DialScene::CALL_NORMAL) {
        TELEPHONY_LOGW("emergency call not support video upgrade");
        isSupportVideoCall = false;
    }
    return isSupportVideoCall;
}

int32_t CallPolicy::AnswerCallPolicy(int32_t callId, int32_t videoState)
{
    if (videoState != static_cast<int32_t>(VideoStateType::TYPE_VOICE) &&
        videoState != static_cast<int32_t>(VideoStateType::TYPE_VIDEO)) {
        TELEPHONY_LOGE("videoState is invalid!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    TelCallState state = GetCallState(callId);
    if (state != TelCallState::CALL_STATUS_INCOMING && state != TelCallState::CALL_STATUS_WAITING) {
        TELEPHONY_LOGE("current call state is:%{public}d, accept call not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::RejectCallPolicy(int32_t callId)
{
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    TelCallState state = GetCallState(callId);
    if (state != TelCallState::CALL_STATUS_INCOMING && state != TelCallState::CALL_STATUS_WAITING) {
        TELEPHONY_LOGE("current call state is:%{public}d, reject call not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::HoldCallPolicy(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("GetOneCallObject failed, this callId is invalid! callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (call->GetCallRunningState() != CallRunningState::CALL_RUNNING_STATE_ACTIVE) {
        TELEPHONY_LOGE("this call is not activated! callId:%{public}d", callId);
        return CALL_ERR_CALL_IS_NOT_ACTIVATED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::UnHoldCallPolicy(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("GetOneCallObject failed, this callId is invalid! callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (call->GetCallRunningState() != CallRunningState::CALL_RUNNING_STATE_HOLD) {
        TELEPHONY_LOGE("this call is not on holding state! callId:%{public}d", callId);
        return CALL_ERR_CALL_IS_NOT_ON_HOLDING;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::HangUpPolicy(int32_t callId)
{
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    TelCallState state = GetCallState(callId);
    if (state == TelCallState::CALL_STATUS_IDLE || state == TelCallState::CALL_STATUS_DISCONNECTING ||
        state == TelCallState::CALL_STATUS_DISCONNECTED) {
        TELEPHONY_LOGE("current call state is:%{public}d, hang up call not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SwitchCallPolicy(int32_t callId)
{
    std::list<int32_t> callIdList;
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    GetCarrierCallList(callIdList);
    if (callIdList.size() < onlyTwoCall_) {
        callIdList.clear();
        return CALL_ERR_PHONE_CALLS_TOO_FEW;
    }
    if (GetCallState(callId) != TelCallState::CALL_STATUS_HOLDING ||
        IsCallExist(TelCallState::CALL_STATUS_DIALING) || IsCallExist(TelCallState::CALL_STATUS_ALERTING)) {
        TELEPHONY_LOGE("the call is not on hold, callId:%{public}d", callId);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    callIdList.clear();
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::VideoCallPolicy(int32_t callId)
{
    TELEPHONY_LOGI("callid %{public}d", callId);
    sptr<CallBase> callPtr = CallObjectManager::GetOneCallObject(callId);
    if (callPtr == nullptr) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::StartRttPolicy(int32_t callId)
{
    sptr<CallBase> callPtr = CallObjectManager::GetOneCallObject(callId);
    if (callPtr == nullptr) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return CALL_ERR_INVALID_CALLID;
    }
    if (callPtr->GetCallType() != CallType::TYPE_IMS) {
        TELEPHONY_LOGE("calltype is illegal, calltype:%{public}d", callPtr->GetCallType());
        return CALL_ERR_UNSUPPORTED_NETWORK_TYPE;
    }
    TelCallState state = GetCallState(callId);
    if (state != TelCallState::CALL_STATUS_ACTIVE) {
        TELEPHONY_LOGE("current call state is:%{public}d, StartRtt not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::StopRttPolicy(int32_t callId)
{
    sptr<CallBase> callPtr = CallObjectManager::GetOneCallObject(callId);
    if (callPtr == nullptr) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return CALL_ERR_INVALID_CALLID;
    }
    if (callPtr->GetCallType() != CallType::TYPE_IMS) {
        TELEPHONY_LOGE("calltype is illegal, calltype:%{public}d", callPtr->GetCallType());
        return CALL_ERR_UNSUPPORTED_NETWORK_TYPE;
    }
    TelCallState state = GetCallState(callId);
    if (state != TelCallState::CALL_STATUS_ACTIVE) {
        TELEPHONY_LOGE("current call state is:%{public}d, StopRtt not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::IsValidSlotId(int32_t slotId)
{
    bool result = DelayedSingleton<CallNumberUtils>::GetInstance()->IsValidSlotId(slotId);
    if (!result) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::EnableVoLtePolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::DisableVoLtePolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::IsVoLteEnabledPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::VoNRStatePolicy(int32_t slotId, int32_t state)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    if (state != static_cast<int32_t>(VoNRState::VONR_STATE_ON) &&
        state != static_cast<int32_t>(VoNRState::VONR_STATE_OFF)) {
        TELEPHONY_LOGE("invalid state!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::GetCallWaitingPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SetCallWaitingPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::GetCallRestrictionPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SetCallRestrictionPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::GetCallTransferInfoPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SetCallTransferInfoPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SetCallPreferenceModePolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::GetImsConfigPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SetImsConfigPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::GetImsFeatureValuePolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SetImsFeatureValuePolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::InviteToConferencePolicy(int32_t callId, std::vector<std::string> &numberList)
{
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    // check number legality
    if (numberList.empty()) {
        TELEPHONY_LOGE("empty phone number list!");
        return CALL_ERR_PHONE_NUMBER_EMPTY;
    }
    for (size_t index = 0; index < numberList.size(); ++index) {
        if (numberList[index].empty()) {
            TELEPHONY_LOGE("empty phone number !");
            return CALL_ERR_PHONE_NUMBER_EMPTY;
        }
        if (numberList[index].length() > kMaxNumberLen) {
            TELEPHONY_LOGE("phone number too long !");
            return CALL_ERR_NUMBER_OUT_OF_RANGE;
        }
    }
    if (DelayedSingleton<ImsConference>::GetInstance()->GetMainCall() != callId) {
        TELEPHONY_LOGE("conference with main callId %{public}d not exist", callId);
        return CALL_ERR_CONFERENCE_NOT_EXISTS;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::CloseUnFinishedUssdPolicy(int32_t slotId)
{
    if (IsValidSlotId(slotId) != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SetEdmPolicy(bool isDialingTrustlist, const std::vector<std::string> &dialingList,
    bool isIncomingTrustlist, const std::vector<std::string> &incomingList)
{
    return edmCallPolicy_->SetCallPolicy(isDialingTrustlist, dialingList, isIncomingTrustlist, incomingList);
}

bool CallPolicy::IsDialingEnable(const std::string &phoneNum)
{
    return edmCallPolicy_->IsDialingEnable(phoneNum);
}

bool CallPolicy::IsIncomingEnable(const std::string &phoneNum)
{
    return edmCallPolicy_->IsIncomingEnable(phoneNum);
}
} // namespace Telephony
} // namespace OHOS
