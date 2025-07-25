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

#include "call_broadcast_subscriber.h"

#include <string_ex.h>

#include "audio_device_manager.h"
#include "call_manager_errors.h"
#include "telephony_log_wrapper.h"
#include "call_control_manager.h"
#include "satellite_call_control.h"
#include "securec.h"
#include "call_superprivacy_control_manager.h"
#include "call_connect_ability.h"
#include "call_ability_connect_callback.h"
#include "number_identity_service.h"
#include "os_account_manager.h"
#include "call_object_manager.h"
#include "bluetooth_call_connection.h"

namespace OHOS {
namespace Telephony {
using namespace OHOS::EventFwk;
static constexpr int16_t INCOMING_CALL_MISSED_CODE = 0;
static constexpr int16_t PUBLISH_MISSCALL_EVENT_DELAY_TIME = 2000;
CallBroadcastSubscriber::CallBroadcastSubscriber(const OHOS::EventFwk::CommonEventSubscribeInfo &subscriberInfo)
    : CommonEventSubscriber(subscriberInfo)
{
    memberFuncMap_[UNKNOWN_BROADCAST_EVENT] =
        [this](const EventFwk::CommonEventData &data) { UnknownBroadcast(data); };
    memberFuncMap_[SIM_STATE_BROADCAST_EVENT] =
        [this](const EventFwk::CommonEventData &data) { SimStateBroadcast(data); };
    memberFuncMap_[CONNECT_CALLUI_SERVICE] =
        [this](const EventFwk::CommonEventData &data) { ConnectCallUiServiceBroadcast(data); };
    memberFuncMap_[HIGH_TEMP_LEVEL_CHANGED] =
        [this](const EventFwk::CommonEventData &data) { HighTempLevelChangedBroadcast(data); };
    memberFuncMap_[SUPER_PRIVACY_MODE] =
        [this](const EventFwk::CommonEventData &data) { ConnectCallUiSuperPrivacyModeBroadcast(data); };
    memberFuncMap_[BLUETOOTH_REMOTEDEVICE_NAME_UPDATE] =
        [this](const EventFwk::CommonEventData &data) { UpdateBluetoothDeviceName(data); };
    memberFuncMap_[USER_SWITCHED] =
        [this](const EventFwk::CommonEventData &data) { ConnectCallUiUserSwitchedBroadcast(data); };
    memberFuncMap_[SHUTDOWN] =
        [this](const EventFwk::CommonEventData &data) { ShutdownBroadcast(data); };
    memberFuncMap_[HSDR_EVENT] =
        [this](const EventFwk::CommonEventData &data) { HsdrEventBroadcast(data); };
    memberFuncMap_[HFP_EVENT] =
        [this](const EventFwk::CommonEventData &data) { HfpConnectBroadcast(data); };
    memberFuncMap_[SCREEN_UNLOCKED] =
        [this](const EventFwk::CommonEventData &data) { ScreenUnlockedBroadcast(data); };
    memberFuncMap_[MUTE_KEY_PRESS] =
        [this](const EventFwk::CommonEventData &data) { MuteKeyBroadcast(data); };
}

void CallBroadcastSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    uint32_t code = UNKNOWN_BROADCAST_EVENT;
    OHOS::EventFwk::Want want = data.GetWant();
    std::string action = data.GetWant().GetAction();
    TELEPHONY_LOGI("receive one broadcast:%{public}s", action.c_str());
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SIM_STATE_CHANGED) {
        code = SIM_STATE_BROADCAST_EVENT;
    } else if (action == "event.custom.contacts.PAGE_STATE_CHANGE") {
        code = CONNECT_CALLUI_SERVICE;
    } else if (action == "usual.event.thermal.satcomm.HIGH_TEMP_LEVEL") {
        code = HIGH_TEMP_LEVEL_CHANGED;
    } else if (action == "usual.event.SUPER_PRIVACY_MODE") {
        code = SUPER_PRIVACY_MODE;
    } else if (action == "usual.event.HSDR_EVENT") {
        code = HSDR_EVENT;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_BLUETOOTH_REMOTEDEVICE_NAME_UPDATE) {
        code = BLUETOOTH_REMOTEDEVICE_NAME_UPDATE;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        code = USER_SWITCHED;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN) {
        code = SHUTDOWN;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        code = SCREEN_UNLOCKED;
    } else if (action == "usual.event.bluetooth.CONNECT_HFP_HF") {
        code = HFP_EVENT;
    } else if (action == "multimodal.event.MUTE_KEY_PRESS") {
        code = MUTE_KEY_PRESS;
    } else {
        code = UNKNOWN_BROADCAST_EVENT;
    }
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return memberFunc(data);
        }
    }
}

void CallBroadcastSubscriber::UnknownBroadcast(const EventFwk::CommonEventData &data)
{
    TELEPHONY_LOGI("you receive one unknown broadcast!");
}

void CallBroadcastSubscriber::SimStateBroadcast(const EventFwk::CommonEventData &data)
{
    TELEPHONY_LOGI("sim state broadcast code:%{public}d", data.GetCode());
}

void CallBroadcastSubscriber::ConnectCallUiServiceBroadcast(const EventFwk::CommonEventData &data)
{
    bool isConnectService = data.GetWant().GetBoolParam("isShouldConnect", false);
    TELEPHONY_LOGI("isConnectService:%{public}d", isConnectService);
    DelayedSingleton<CallControlManager>::GetInstance()->ConnectCallUiService(isConnectService);
}

void CallBroadcastSubscriber::HighTempLevelChangedBroadcast(const EventFwk::CommonEventData &data)
{
    int32_t satcommHighTempLevel = data.GetWant().GetIntParam("satcomm_high_temp_level", -1);
    TELEPHONY_LOGI("satcommHighTempLevel:%{public}d", satcommHighTempLevel);
    DelayedSingleton<SatelliteCallControl>::GetInstance()->SetSatcommTempLevel(satcommHighTempLevel);
}

void CallBroadcastSubscriber::ConnectCallUiSuperPrivacyModeBroadcast(const EventFwk::CommonEventData &data)
{
    bool isInCall = data.GetWant().GetBoolParam("isInCall", false);
    bool isHangup = data.GetWant().GetBoolParam("isHangup", false);
    if (isInCall && isHangup) {
        DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->CloseAllCall();
        return;
    }
    int32_t videoState = data.GetWant().GetIntParam("videoState", -1);
    bool isAnswer = data.GetWant().GetBoolParam("isAnswer", false);
    DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->SetOldSuperPrivacyMode();
    DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->SetIsChangeSuperPrivacyMode(true);
    if (isInCall) {
        DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->CloseSuperPrivacyMode();
        return;
    }
    TELEPHONY_LOGI("CallUiSuperPrivacyModeBroadcast isAnswer:%{public}d", isAnswer);
    if (isAnswer) {
        int32_t callId = data.GetWant().GetIntParam("callId", -1);
        TELEPHONY_LOGI("CallUiSuperPrivacyModeBroadcast_Answer callId:%{public}d", callId);
        DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->
            CloseAnswerSuperPrivacyMode(callId, videoState);
    } else {
        std::string phoneNumber = data.GetWant().GetStringParam("phoneNumber");
        std::u16string phNumber = Str8ToStr16(phoneNumber);
        int32_t accountId = data.GetWant().GetIntParam("accountId", -1);
        TELEPHONY_LOGI("CallUiSuperPrivacyModeBroadcast_Answer accountId:%{public}d", accountId);
        int32_t dialScene = data.GetWant().GetIntParam("dialScene", -1);
        int32_t dialType = data.GetWant().GetIntParam("dialType", -1);
        int32_t callType = data.GetWant().GetIntParam("callType", -1);
        DelayedSingleton<CallSuperPrivacyControlManager>::GetInstance()->CloseCallSuperPrivacyMode(
            phNumber, accountId, videoState, dialScene, dialType, callType);
    }
}

void CallBroadcastSubscriber::UpdateBluetoothDeviceName(const EventFwk::CommonEventData &data)
{
    std::string macAddress = data.GetWant().GetStringParam("deviceAddr");
    std::string deviceName = data.GetWant().GetStringParam("remoteName");
    DelayedSingleton<AudioDeviceManager>::GetInstance()->UpdateBluetoothDeviceName(macAddress, deviceName);
}

void CallBroadcastSubscriber::ConnectCallUiUserSwitchedBroadcast(const EventFwk::CommonEventData &data)
{
    if (!DelayedSingleton<CallConnectAbility>::GetInstance()->GetConnectFlag()) {
        TELEPHONY_LOGE("is not connected");
        return;
    }
    TELEPHONY_LOGI("User switched, need reconnect ability");
    DelayedSingleton<CallConnectAbility>::GetInstance()->DisconnectAbility();
    sptr<CallAbilityConnectCallback> connectCallback_ = new CallAbilityConnectCallback();
    connectCallback_->ReConnectAbility();
    DelayedSingleton<CallControlManager>::GetInstance()->StopFlashRemind();
}

void CallBroadcastSubscriber::ShutdownBroadcast(const EventFwk::CommonEventData &data)
{
    TELEPHONY_LOGI("system is shutdown.");
    DelayedSingleton<CallControlManager>::GetInstance()->DisconnectAllCalls();
    DelayedSingleton<CallControlManager>::GetInstance()->StopFlashRemind();
}

void CallBroadcastSubscriber::HsdrEventBroadcast(const EventFwk::CommonEventData &data)
{
    TELEPHONY_LOGI("HsdrEventBroadcast begin");
    auto dataStr = data.GetData();
    if (dataStr.find("com.numberidentity.devicenumber") != std::string::npos) {
        DelayedRefSingleton<NumberIdentityServiceHelper>::GetInstance().NotifyNumberMarkDataUpdate();
    }
    TELEPHONY_LOGI("HsdrEventBroadcast end");
}

void CallBroadcastSubscriber::HfpConnectBroadcast(const EventFwk::CommonEventData &data)
{
    TELEPHONY_LOGI("HfpConnectBroadcast begin");
    std::string phoneNumber = data.GetWant().GetStringParam("phoneNumber");
    std::string contactName = data.GetWant().GetStringParam("contact");
    DelayedSingleton<CallConnectAbility>::GetInstance()->ConnectAbility();
    constexpr static uint64_t DISCONNECT_DELAY_TIME = 3000000;
    DelayedSingleton<CallObjectManager>::GetInstance()->DelayedDisconnectCallConnectAbility(DISCONNECT_DELAY_TIME);
    if (contactName.empty()) {
        TELEPHONY_LOGI("contactName is empty.");
        return;
    }
    sptr<CallBase> foregroundCall = CallObjectManager::GetForegroundCall(false);
    if (foregroundCall == nullptr || phoneNumber != foregroundCall->GetAccountNumber()) {
        TELEPHONY_LOGI("foregroundCall is nullptr.");
        DelayedSingleton<BluetoothCallConnection>::GetInstance()->SetHfpContactName(phoneNumber, contactName);
    } else {
        TELEPHONY_LOGI("need SetCallerInfo.");
        ContactInfo contactInfo = foregroundCall->GetCallerInfo();
        contactInfo.name = contactName;
        contactInfo.isContacterExists = true;
        foregroundCall->SetCallerInfo(contactInfo);
    }
    TELEPHONY_LOGI("HfpConnectBroadcast end");
}

void CallBroadcastSubscriber::ScreenUnlockedBroadcast(const EventFwk::CommonEventData &data)
{
    int32_t userId = 0;
    bool isUserUnlocked = false;
    AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
    AccountSA::OsAccountManager::IsOsAccountVerified(userId, isUserUnlocked);
    TELEPHONY_LOGI("isUserUnlocked: %{public}d", isUserUnlocked);
    if (!isUserUnlocked) {
        std::this_thread::sleep_for(std::chrono::milliseconds(PUBLISH_MISSCALL_EVENT_DELAY_TIME));
        AAFwk::Want want;
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_INCOMING_CALL_MISSED);
        EventFwk::CommonEventData eventData;
        eventData.SetWant(want);
        eventData.SetCode(INCOMING_CALL_MISSED_CODE);
        EventFwk::CommonEventPublishInfo publishInfo;
        publishInfo.SetOrdered(true);
        EventFwk::CommonEventManager::PublishCommonEvent(eventData, publishInfo, nullptr);
    } else {
        DelayedSingleton<CallControlManager>::GetInstance()->StopFlashRemind();
    }
}

void CallBroadcastSubscriber::MuteKeyBroadcast(const EventFwk::CommonEventData &data)
{
    DelayedSingleton<CallControlManager>::GetInstance()->StopFlashRemind();
}
} // namespace Telephony
} // namespace OHOS
