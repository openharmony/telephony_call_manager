/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef CALL_CONTROL_MANAGER_H
#define CALL_CONTROL_MANAGER_H

#include <cstring>
#include <list>
#include <memory>
#include <mutex>

#include "app_state_observer.h"
#include "call_broadcast_subscriber.h"
#include "call_policy.h"
#include "call_request_handler.h"
#include "call_setting_manager.h"
#include "call_state_listener.h"
#include "incoming_call_wake_up.h"
#include "missed_call_notification.h"
#include "pac_map.h"
#include "singleton.h"
#include "system_ability_status_change_stub.h"
#include "ffrt.h"

#ifdef ABILITY_POWER_SUPPORT
#include "power_mgr_client.h"
#endif

/**
 * Singleton
 * @ClassName:CallControlManager
 * @Description:CallControlManager is designed for performing dial/answer/reject etc ops
 * on kinds of calls(ims,cs,ott). usually as an entrance for downflowed [app->ril] telephony business
 */
namespace OHOS {
namespace Telephony {
    constexpr const char *KEY_CONST_TELEPHONY_READ_SET_VOIP_CALL_INFO =
        "const.telephony.read_set_voip_call_info";
    constexpr int32_t WEAR_STATUS_INVALID = 0;
    constexpr int32_t WEAR_STATUS_OFF = 1;
    constexpr int32_t WEAR_STATUS_ON = 2;
class WearStatusObserver : public AAFwk::DataAbilityObserverStub {
public:
    WearStatusObserver() = default;
    ~WearStatusObserver() = default;
    void OnChange() override;
};
class IncomingFlashReminder;
class CallControlManager : public CallPolicy {
    DECLARE_DELAYED_SINGLETON(CallControlManager)

public:
    bool Init();
    void UnInit();
    int32_t DialCall(std::u16string &number, AppExecFwk::PacMap &extras);
    int32_t AnswerCall(int32_t callId, int32_t videoState);
    int32_t HandlerAnswerCall(int32_t callId, int32_t videoState);
    int32_t RejectCall(int32_t callId, bool rejectWithMessage, std::u16string textMessage);
    int32_t HangUpCall(int32_t callId);
    int32_t GetCallState();
    int32_t HoldCall(int32_t callId);
    int32_t UnHoldCall(int32_t callId);
    int32_t SwitchCall(int32_t callId);
    bool HasCall();
    bool HasVoipCall();
    int32_t GetMeetimeCallState();
    int32_t IsNewCallAllowed(bool &enabled);
    int32_t IsRinging(bool &enabled);
    int32_t HasEmergency(bool &enabled);
    bool NotifyNewCallCreated(sptr<CallBase> &callObjectPtr);
    bool NotifyCallDestroyed(const DisconnectedDetails &details);
    bool NotifyCallStateUpdated(sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState);
    bool NotifyVoipCallStateUpdated(CallAttributeInfo info, TelCallState priorState, TelCallState nextState);
    bool NotifyIncomingCallAnswered(sptr<CallBase> &callObjectPtr);
    bool NotifyIncomingCallRejected(sptr<CallBase> &callObjectPtr, bool isSendSms, std::string content);
    bool NotifyCallEventUpdated(CallEventInfo &info);
    int32_t StartDtmf(int32_t callId, char str);
    int32_t StopDtmf(int32_t callId);
    int32_t PostDialProceed(int32_t callId, bool proceed);
    int32_t GetCallWaiting(int32_t slotId);
    int32_t SetCallWaiting(int32_t slotId, bool activate);
    int32_t GetCallRestriction(int32_t slotId, CallRestrictionType type);
    int32_t SetCallRestriction(int32_t slotId, CallRestrictionInfo &info);
    int32_t SetCallRestrictionPassword(
        int32_t slotId, CallRestrictionType fac, const char *oldPassword, const char *newPassword);
    int32_t GetCallTransferInfo(int32_t slotId, CallTransferType type);
    int32_t SetCallTransferInfo(int32_t slotId, CallTransferInfo &info);
    int32_t CanSetCallTransferTime(int32_t slotId, bool &result);
    int32_t SetCallPreferenceMode(int32_t slotId, int32_t mode);
    // merge calls
    int32_t CombineConference(int32_t mainCallId);
    int32_t SeparateConference(int32_t callId);
    int32_t KickOutFromConference(int32_t callId);
    int32_t GetMainCallId(int32_t callId, int32_t &mainCallId);
    int32_t GetSubCallIdList(int32_t callId, std::vector<std::u16string> &callIdList);
    int32_t GetCallIdListForConference(int32_t callId, std::vector<std::u16string> &callIdList);
    int32_t GetImsConfig(int32_t slotId, ImsConfigItem item);
    int32_t SetImsConfig(int32_t slotId, ImsConfigItem item, std::u16string &value);
    int32_t GetImsFeatureValue(int32_t slotId, FeatureType type);
    int32_t SetImsFeatureValue(int32_t slotId, FeatureType type, int32_t value);
    int32_t EnableImsSwitch(int32_t slotId);
    int32_t DisableImsSwitch(int32_t slotId);
    int32_t IsImsSwitchEnabled(int32_t slotId, bool &enabled);
    int32_t SetVoNRState(int32_t slotId, int32_t state);
    int32_t GetVoNRState(int32_t slotId, int32_t &state);
    int32_t UpdateImsCallMode(int32_t callId, ImsCallMode mode);
    int32_t StartRtt(int32_t callId, std::u16string &msg);
    int32_t StopRtt(int32_t callId);
    // invite calls to participate conference
    int32_t JoinConference(int32_t callId, std::vector<std::u16string> &numberList);
    int32_t SetMuted(bool isMute);
    int32_t MuteRinger();
    int32_t SetAudioDevice(const AudioDevice &audioDevice);
    int32_t ControlCamera(std::u16string cameraId, int32_t callingUid, int32_t callingPid);
    int32_t SetPreviewWindow(VideoWindow &window);
    int32_t SetDisplayWindow(VideoWindow &window);
    int32_t SetCameraZoom(float zoomRatio);
    int32_t SetPausePicture(std::u16string path);
    int32_t SetDeviceDirection(int32_t rotation);
    int32_t IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled);
    int32_t FormatPhoneNumber(std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber);
    int32_t FormatPhoneNumberToE164(
        std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber);
    int32_t CloseUnFinishedUssd(int32_t slotId);
    void GetDialParaInfo(DialParaInfo &info);
    void GetDialParaInfo(DialParaInfo &info, AppExecFwk::PacMap &extras);
    void ConnectCallUiService(bool shouldConnect);
    bool ShouldDisconnectService();
    int32_t RemoveMissedIncomingCallNotification();
    int32_t SetVoIPCallState(int32_t state);
    int32_t GetVoIPCallState(int32_t &state);
    int32_t SetVoIPCallInfo(int32_t callId, int32_t state, std::string phoneNumber);
    int32_t GetVoIPCallInfo(int32_t &callId, int32_t &state, std::string &phoneNumber);
    int32_t AddCallLogAndNotification(sptr<CallBase> &callObjectPtr);
    int32_t AddBlockLogAndNotification(sptr<CallBase> &callObjectPtr);
    int32_t HangUpVoipCall();
    int32_t CarrierAndVoipConflictProcess(int32_t callId, TelCallState callState);
    void AcquireIncomingLock();
    void ReleaseIncomingLock();
    void AcquireDisconnectedLock();
    void ReleaseDisconnectedLock();
    void DisconnectAllCalls();
    void StartFlashRemind();
    void StopFlashRemind();
    void ClearFlashReminder();
#ifdef NOT_SUPPORT_MULTICALL
    bool HangUpFirstCallBtAndESIM(int32_t secondCallId);
    bool HangUpFirstCallBtCall(int32_t secondCallId);
    bool HangUpFirstCallESIMCall(int32_t secondCallId);
    bool HangUpFirstCall(int32_t secondCallId);
    void HangUpFirstCallBySecondCallID(int32_t secondCallId, bool secondAutoAnswer = false);
#endif
    bool IsNotWearOnWrist();
    void SetWearState(int32_t state);
    void RegisterObserver();
    void UnRegisterObserver();
    void HandleVideoRingPlayFail();
private:
    void CallStateObserve();
    int32_t NumberLegalityCheck(std::string &number);
    int32_t SubscriberSaStateChange();
    void ReportPhoneUEInSuperPrivacy(const std::string &eventName);
    void PackageDialInformation(AppExecFwk::PacMap &extras, std::string accountNumber, bool isEcc);
    static void handler();
    bool cancel(ffrt::task_handle &handle);
    int32_t CanDial(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc);
    void AnswerHandlerForSatelliteOrVideoCall(sptr<CallBase> &call, int32_t videoState);
    bool CurrentIsSuperPrivacyMode(int32_t callId, int32_t videoState);
    void AppStateObserver();
    void SetCallTypeExtras(AppExecFwk::PacMap &extras);
    void HandleVoipConnected(int32_t &numActive, int32_t callId);
    void HandleVoipIncoming(int32_t &numActive, int32_t callId, const std::string phoneNumber);
    int32_t HandleVoipDisconnected(int32_t &numActive, int32_t numHeld, int32_t callId,
        int32_t state, const std::string phoneNumber);
    void HandleVoipAlerting(int32_t callId, const std::string phoneNumber);
    bool IsSupportSetVoipInfo();
    void SetVoipCallInfoInner(const int32_t callId, const int32_t state,
        const std::string phoneNumber);
    void sendEventToVoip(CallAbilityEventId eventId);
    bool IsCallActivated(const TelCallState& priorState, const TelCallState& nextState);
private:
    class SystemAbilityListener : public SystemAbilityStatusChangeStub {
    public:
        explicit SystemAbilityListener();
        ~SystemAbilityListener() = default;
        void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
        void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    private:
        void CommonBroadcastSubscriber();
        void ContactsBroadcastSubscriber();
        void SatcommBroadcastSubscriber();
        void SuperPrivacyModeBroadcastSubscriber();
        void HSDRBroadcastSubscriber();
        void HfpBroadcastSubscriber();
        void MuteKeyBroadcastSubscriber();

    private:
        std::vector<std::shared_ptr<CallBroadcastSubscriber>> subscriberPtrList_;
    };

private:
    std::unique_ptr<CallStateListener> callStateListenerPtr_;
    std::unique_ptr<CallRequestHandler> CallRequestHandlerPtr_;
    // notify when incoming calls are ignored, not rejected or answered
    std::shared_ptr<IncomingCallWakeup> incomingCallWakeup_;
#ifdef ABILITY_POWER_SUPPORT
    std::shared_ptr<PowerMgr::RunningLock> disconnectedRunningLock_;
#endif
    const int32_t DISCONNECTED_LOCK_TIMEOUT = 2000;
    std::shared_ptr<MissedCallNotification> missedCallNotification_;
    std::unique_ptr<CallSettingManager> callSettingManagerPtr_;
    sptr<ISystemAbilityStatusChange> statusChangeListener_ = nullptr;
    DialParaInfo dialSrcInfo_;
    AppExecFwk::PacMap extras_;
    std::mutex mutex_;
    CallStateToApp VoIPCallState_ = CallStateToApp::CALL_STATE_IDLE;
    bool shouldDisconnect = true;
    static bool alarmSeted;
    struct AnsweredCallQueue {
        bool hasCall = false;
        int32_t callId = 0;
        int32_t videoState = 0;
    } AnsweredCallQueue_;

    struct VoipCallInfo {
        int32_t callId = 10000;
        int32_t state = 0;
        std::string phoneNumber = "";
    } VoipCallInfo_;

    ffrt::task_handle disconnectHandle = nullptr;
    sptr<ApplicationStateObserver> appStateObserver = nullptr;
    sptr<AppExecFwk::IAppMgr> appMgrProxy = nullptr;
    
    std::mutex voipMutex_;
    sptr<WearStatusObserver> wearStatusObserver_ = nullptr;
    int32_t wearStatus_ = WEAR_STATUS_INVALID;
    std::mutex wearStatusMutex_;
    ffrt::mutex reminderMutex_;
    std::shared_ptr<IncomingFlashReminder> incomingFlashReminder_ {nullptr};
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_CONTROL_MANAGER_H
