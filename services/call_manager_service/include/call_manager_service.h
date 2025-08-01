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

#ifndef CALL_MANAGER_SERVICE_H
#define CALL_MANAGER_SERVICE_H

#include <memory>

#include "bluetooth_call_manager.h"
#include "call_control_manager.h"
#include "call_manager_service_stub.h"
#include "call_state_report_proxy.h"
#include "call_status_manager.h"
#include "i_call_status_callback.h"
#include "iremote_stub.h"
#include "iservice_registry.h"
#include "singleton.h"
#include "system_ability.h"
#include "system_ability_definition.h"
#include "bluetooth_call_state.h"

namespace OHOS {
namespace Telephony {
class CallManagerService : public SystemAbility,
                           public CallManagerServiceStub,
                           public std::enable_shared_from_this<CallManagerService> {
    DECLARE_DELAYED_SINGLETON(CallManagerService)
    DECLARE_SYSTEM_ABILITY(CallManagerService)
public:
    void SetCallStatusManager(std::shared_ptr<CallStatusManager> callStatusManager);
    bool Init();
    void UnInit();

    /**
     * service OnStart
     */
    void OnStart() override;

    /**
     * service OnStop
     */
    void OnStop() override;

    /**
     * service dump
     * @param fd
     * @param args
     * @return
     */
    int32_t Dump(std::int32_t fd, const std::vector<std::u16string> &args) override;

    /**
     * Get bindTime_
     * @return string
     */
    std::string GetBindTime();

    /**
     * Get spendTime_
     * @return string
     */
    std::string GetStartServiceSpent();

    /**
     * Get serviceRunningState_
     * @return serviceRunningState_
     */
    int32_t GetServiceRunningState();

    /**
     * RegisterCallBack
     *
     * @brief Register callback
     * @param callback[in], callback function pointer
     * @param bundleName[in], bundle name
     * @return Returns 0 on success, others on failure.
     */
    int32_t RegisterCallBack(const sptr<ICallAbilityCallback> &callback) override;

    /**
     * UnRegisterCallBack
     *
     * @brief unregister callback
     * @param bundleName[in], bundle name
     * @return Returns 0 on success, others on failure.
     */
    int32_t UnRegisterCallBack() override;

    /**
     * ObserverOnCallDetailsChange
     *
     * @brief excute when observe OnCallDetailsChange
     * @return Returns 0 on success, others on failure.
     */
    int32_t ObserverOnCallDetailsChange() override;

    /**
     * DialCall
     *
     * @brief Dial a phone call
     * @param number[in], dial param.
     * @param extras[in], extras date.
     * @return Returns callId when the value is greater than zero, others on failure.
     */
    int32_t DialCall(std::u16string number, AppExecFwk::PacMap &extras) override;

    /**
     * MakeCall
     *
     * @brief Make a phone call
     * @param number[in], call param.
     * @return Returns 0 on success, others on failure.
     */
    int32_t MakeCall(std::string number) override;

    /**
     * AnswerCall
     *
     * @brief Answer a phone call
     * @param callId[in], call id
     * @param videoState[in], 0: audio, 1: video
     * @return Returns 0 on success, others on failure.
     */
    int32_t AnswerCall(int32_t callId, int32_t videoState) override;

    /**
     * RejectCall
     *
     * @brief Reject a phone call
     * @param callId[in], call id
     * @param rejectWithMessage[in], Whether to enter the reason for rejection,true:yes false:no
     * @param textMessage[in], The reason you reject the call
     * @return Returns 0 on success, others on failure.
     */
    int32_t RejectCall(int32_t callId, bool rejectWithMessage, std::u16string textMessage) override;

    /**
     * HangUpCall
     *
     * @brief Hang up the phone
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t HangUpCall(int32_t callId) override;

    /**
     * GetCallState
     *
     * @brief Obtain the call status of the device
     * @return Returns call state.
     */
    int32_t GetCallState() override;

    /**
     * HoldCall
     *
     * @brief Park a phone call
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t HoldCall(int32_t callId) override;

    /**
     * UnHoldCall
     *
     * @brief Activate a phone call
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t UnHoldCall(int32_t callId) override;

    /**
     * SwitchCall
     *
     * @brief Switch the phone
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t SwitchCall(int32_t callId) override;

    /**
     * HasCall
     *
     * @brief Is there Call
     * @param isInCludeVoipCall[in], include voip call or not
     * @return Returns true on has call, others on there is no call.
     */
    bool HasCall(const bool isInCludeVoipCall = true) override;

    /**
     * IsNewCallAllowed
     *
     * @brief Can I initiate a call
     * @param enabled[out], true on can, others on there is not can
     * @return Returns interface processing results.
     */
    int32_t IsNewCallAllowed(bool &enabled) override;

    /**
     * SetMuted
     *
     * @brief Mute the Microphone
     * @param isMute[in], mute state
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetMuted(bool isMute) override;

    /**
     * MuteRinger
     *
     * @brief Call mute
     * @return Returns 0 on success, others on failure.
     */
    int32_t MuteRinger() override;

    /**
     * SetAudioDevice
     *
     * @brief Setting the Audio Channel
     * @param audioDevice[in], contain audioDeviceType ande address
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetAudioDevice(const AudioDevice &audioDevice) override;

    /**
     * IsRinging
     *
     * @brief Whether the ringing
     * @param enabled[out], true on ringing, false on there is no ringing
     * @return Returns interface processing results.
     */
    int32_t IsRinging(bool &enabled) override;

    /**
     * IsInEmergencyCall
     *
     * @brief Is there an emergency call
     * @param enabled[out], true on emergency call, false on no emergency call
     * @return Returns interface processing results.
     */
    int32_t IsInEmergencyCall(bool &enabled) override;

    /**
     * StartDtmf
     *
     * @brief Enable and send DTMF
     * @param callId[in], call id
     * @param str[in], Characters sent
     * @return Returns 0 on success, others on failure.
     */
    int32_t StartDtmf(int32_t callId, char str) override;

    /**
     * StopDtmf
     *
     * @brief Stop the DTMF
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t StopDtmf(int32_t callId) override;

    int32_t PostDialProceed(int32_t callId, bool proceed) override;

    /**
     * GetCallWaiting
     *
     * @brief Gets whether the call waiting service of the current account is enabled
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallWaiting(int32_t slotId) override;

    /**
     * SetCallWaiting
     *
     * @brief Set the call waiting function for the current account
     * @param slotId[in], The slot id
     * @param activate[in], Activation of switch
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallWaiting(int32_t slotId, bool activate) override;

    /**
     * GetCallRestriction
     *
     * @brief Gets the call restriction information of the specified account
     * @param slotId[in], The slot id
     * @param type[in], Call Restriction type
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallRestriction(int32_t slotId, CallRestrictionType type) override;

    /**
     * SetCallRestriction
     *
     * @brief Set the call restriction function for the current account
     * @param slotId[in], The slot id
     * @param info[in], Call restriction information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallRestriction(int32_t slotId, CallRestrictionInfo &info) override;

    /**
     * SetCallRestrictionPassword
     *
     * @brief Set the call restriction password function for the current account
     * @param slotId[in], The slot id
     * @param fac[in], Call restriction type
     * @param oldPassword[in], Old password of call restriction type
     * @param newPassword[in], New password of call restriction type
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallRestrictionPassword(
        int32_t slotId, CallRestrictionType fac, const char *oldPassword, const char *newPassword) override;

    /**
     * GetCallTransferInfo
     *
     * @brief Gets the call transfer information of the current account
     * @param slotId[in], The slot id
     * @param type[in], Call Transfer Type
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallTransferInfo(int32_t slotId, CallTransferType type) override;

    /**
     * SetCallTransferInfo
     *
     * @brief Set the call transfer function for the current account
     * @param slotId[in], The slot id
     * @param info[in], Call Transfer Information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallTransferInfo(int32_t slotId, CallTransferInfo &info) override;

    /**
     * CanSetCallTransferTime
     *
     * @brief confirm whether IMS support call transfer due to time.
     * @param slotId[in], The slot id
     * @param result[out], The result of support or not
     * @return Returns 0 on success, others on failure.
     */
    int32_t CanSetCallTransferTime(int32_t slotId, bool &result) override;

    /**
     * CombineConference
     *
     * @brief Merge calls to form a conference
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t CombineConference(int32_t mainCallId) override;

    /**
     * SeparateConference
     *
     * @brief Separates a specified call from a conference call
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t SeparateConference(int32_t callId) override;

    /**
     * KickOutFromConference
     *
     * @brief Hangup a specified call from a conference call
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t KickOutFromConference(int32_t callId) override;

    /**
     * ControlCamera
     *
     * @brief Open or close camera
     * @param callId[in], call id
     * @param cameraId[in], The camera id
     * @return Returns 0 on success, others on failure.
     */
    int32_t ControlCamera(int32_t callId, std::u16string &cameraId) override;

    /**
     * SetPreviewWindow
     *
     * @brief Set the location and size of the preview window for videos captured by the local camera.
     * @param callId[in], call id
     * @param surfaceId[in], Window information
     * @param surface[in], Window information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetPreviewWindow(int32_t callId, std::string &surfaceId, sptr<Surface> surface) override;

    /**
     * SetDisplayWindow
     *
     * @brief Sets the location and size of the remote video window.
     * @param callId[in], call id
     * @param surfaceId[in], Window information
     * @param surface[in], Window information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetDisplayWindow(int32_t callId, std::string &surfaceId, sptr<Surface> surface) override;

    /**
     * SetCameraZoom
     *
     * @brief Sets the local camera zoom scale
     * @param zoomRatio[in], Camera scale
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCameraZoom(float zoomRatio) override;

    /**
     * SetPausePicture
     *
     * @brief APP sets the screen of the remote video freeze immediately.
     * If the APP does not call this interface when making a video call,
     * the last frame before the remote video freeze is displayed by default
     * @param callId[in], call id
     * @param path[in], Local Picture address
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetPausePicture(int32_t callId, std::u16string &path) override;

    /**
     * SetDeviceDirection
     *
     * @brief Set the rotation Angle of the local device. The default value is 0
     * @param callId[in], call id
     * @param rotation[in], Rotation Angle
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetDeviceDirection(int32_t callId, int32_t rotation) override;

    /**
     * IsEmergencyPhoneNumber
     *
     * @brief Is it an emergency call
     * @param number[in], Phone number to be formatted
     * @param slotId[in], The slot id
     * @param enabled[out] result of is it an emergency call
     * @return Returns 0 on success, others on failure.
     */
    int32_t IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled) override;

    /**
     * FormatPhoneNumber
     *
     * @brief Formatting a phone number
     * @param number[in], Phone number to be formatted
     * @param countryCode[in], Country code of the phone number
     * @param formatNumber[out] Formatting a phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t FormatPhoneNumber(
        std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber) override;

    /**
     * FormatPhoneNumberToE164
     *
     * @brief Formatting a phone number
     * @param number[in]. Phone number to be formatted
     * @param countryCode[in], Country code of the phone number
     * @param formatNumber[out] Formatting a phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t FormatPhoneNumberToE164(
        std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber) override;

    /**
     * GetMainCallId
     *
     * @brief Obtain the ID of the primary call in a conference
     * @param callId[in], Id of a call in a conference
     * @return Returns main call id, -1 on not call id.
     */
    int32_t GetMainCallId(int32_t callId, int32_t &mainCallId) override;

    /**
     * GetSubCallIdList
     *
     * @brief Obtain the list of neutron call ids
     * @param callId[in], Id of a call in a conference
     * @param callIdList[out], the list of neutron call ids
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetSubCallIdList(int32_t callId, std::vector<std::u16string> &callIdList) override;

    /**
     * GetCallIdListForConference
     *
     * @brief Obtain the callId list of all calls in a conference
     * @param callId[in], Id of a call in a conference
     * @param callIdList[out], the callId list of all calls in a conference
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallIdListForConference(int32_t callId, std::vector<std::u16string> &callIdList) override;

    /**
     * SetCallPreferenceMode
     *
     * @brief Setting the Call Type
     * @param slotId[in], The slot id
     * @param mode[in], Preference Mode
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallPreferenceMode(int32_t slotId, int32_t mode) override;

    /**
     * GetImsConfig
     *
     * @brief Obtain the IMS service configuration
     * @param slotId[in], The slot id
     * @param item[in]
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetImsConfig(int32_t slotId, ImsConfigItem item) override;

    /**
     * SetImsConfig
     *
     * @brief Example Set the IMS service configuration
     * @param slotId[in], The slot id
     * @param item[in]
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetImsConfig(int32_t slotId, ImsConfigItem item, std::u16string &value) override;

    /**
     * GetImsFeatureValue
     *
     * @brief Gets the value of the IMS function item of the specified network type
     * @param slotId[in], The slot id
     * @param info[in], FeatureType
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetImsFeatureValue(int32_t slotId, FeatureType type) override;

    /**
     * SetImsFeatureValue
     *
     * @brief Set the value of the IMS function item of the specified network type
     * @param slotId[in], The slot id
     * @param info[in], FeatureType
     * @param value[in]
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetImsFeatureValue(int32_t slotId, FeatureType type, int32_t value) override;

    /**
     * UpdateImsCallMode
     *
     * @brief Setting the Call Mode
     * @param callId[in], The call id
     * @param mode[in], Calling patterns
     * @return Returns 0 on success, others on failure.
     */
    int32_t UpdateImsCallMode(int32_t callId, ImsCallMode mode) override;

    /**
     * EnableImsSwitch
     *
     * @brief Start VoLte
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t EnableImsSwitch(int32_t slotId) override;

    /**
     * DisableImsSwitch
     *
     * @brief Stop VoLte
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t DisableImsSwitch(int32_t slotId) override;

    /**
     * IsImsSwitchEnabled
     *
     * @brief Whether to enable VoLte
     * @param slotId[in], The slot id
     * @param enabled[out], The result of enable or not
     * @return Returns 0 on success, others on failure.
     */
    int32_t IsImsSwitchEnabled(int32_t slotId, bool &enabled) override;

    /**
     * SetVoNRState
     *
     * @brief Set VoNR State
     * @param slotId[in], The slot id
     * @param state[in], The state of VoNR
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetVoNRState(int32_t slotId, int32_t state) override;

    /**
     * GetVoNRState
     *
     * @brief Get VoNR State
     * @param slotId[in], The slot id
     * @param state[out], The result of VoNR state ON or OFF
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetVoNRState(int32_t slotId, int32_t &state) override;

    /**
     * StartRtt
     *
     * @brief Enable and send RTT information
     * @param callId[in], The call id
     * @param msg[in], RTT information
     * @return Returns 0 on success, others on failure.
     */
    int32_t StartRtt(int32_t callId, std::u16string &msg) override;

    /**
     * StopRtt
     *
     * @brief Close the RTT
     * @param callId[in], The call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t StopRtt(int32_t callId) override;

    /**
     * JoinConference
     *
     * @brief Bring someone into a meeting
     * @param callId[in], The call id
     * @param numberList[in], List of calls to join the conference
     * @return Returns 0 on success, others on failure.
     */
    int32_t JoinConference(int32_t callId, std::vector<std::u16string> &numberList) override;

    /**
     * ReportOttCallDetailsInfo
     *
     * @brief report ott call details info
     * @param ottVec[in], ott call status detail info list
     * @return Returns 0 on success, others on failure.
     */
    int32_t ReportOttCallDetailsInfo(std::vector<OttCallDetailsInfo> &ottVec) override;

    /**
     * ReportOttCallEventInfo
     *
     * @brief report ott call details info
     * @param ottVec[in], ott call status detail info list
     * @return Returns 0 on success, others on failure.
     */
    int32_t ReportOttCallEventInfo(OttCallEventInfo &eventInfo) override;

    /**
     * CloseUnFinishedUssd
     *
     * @brief Close Unfinished ussd function for the current account
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t CloseUnFinishedUssd(int32_t slotId) override;

    /**
     * Remove missed incoming call notification.
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t RemoveMissedIncomingCallNotification() override;

    /**
     * SetVoIPCallState
     *
     * @brief Set VoIP Call State
     * @param state[in], The state of VoIP Call
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetVoIPCallState(int32_t state) override;

    /**
     * GetVoIPCallState
     *
     * @brief Get VoIP Call State
     * @param state[out], The result of VoIP Call
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetVoIPCallState(int32_t &state) override;

    /**
     * @brief Set VoIP Call info
     *
     * @param callId[out] Indicates the VoIP Call id
     * @param state[out] Indicates the VoIP Call state
     * @param phoneNumber[out] Indicates the VoIP Call phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetVoIPCallInfo(int32_t callId, int32_t state, std::string phoneNumber) override;

    /**
     * @brief Get VoIP Call Switch Status
     *
     * @param callId[out] Indicates the VoIP Call id
     * @param state[out] Indicates the VoIP Call state
     * @param phoneNumber[out] Indicates the VoIP Call phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetVoIPCallInfo(int32_t &callId, int32_t &state, std::string &phoneNumber) override;

    /**
     * Handle special code from dialer.
     *
     * @param specialCode[in], special code
     * @return Returns 0 on success, others on failure.
     */
    int32_t InputDialerSpecialCode(const std::string &specialCode) override;

    /**
     * GetProxyObjectPtr
     *
     * @brief get callManager proxy object ptr
     * @param proxyType[in], proxy type
     * @return Returns nullptr on failure, others on success.
     */
    sptr<IRemoteObject> GetProxyObjectPtr(CallManagerProxyType proxyType) override;

    /**
     * ReportAudioDeviceInfo
     *
     * @brief report audio device info
     * @return Returns 0 on success, others on failure.
     */
    int32_t ReportAudioDeviceInfo() override;

    /**
     * CancelCallUpgrade
     *
     * @brief cancel upgrade to video call
     * @param callId[in], The call id
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t CancelCallUpgrade(int32_t callId) override;

    /**
     * RequestCameraCapabilities
     *
     * @brief request camera capabilities
     * @param callId[in], The call id
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t RequestCameraCapabilities(int32_t callId) override;

    /**
     * RegisterVoipCallManagerCallback
     *
     * @brief notify voip register callstatus callback
     * @return Returns 0 on success, others on failure.
     */
    int32_t RegisterVoipCallManagerCallback() override;

    /**
     * @brief notify voip unregister callstatus callback
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t UnRegisterVoipCallManagerCallback() override;

    /**
     * @brief send call ui event
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t SendCallUiEvent(int32_t callId, std::string &eventName) override;

    /**
     * OnAddSystemAbility
     *
     * @brief on system ability added
     * @param systemAbilityId
     * @param deviceId
     */
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    /**
     * RegisterBluetoothCallManagerCallbackPtr
     *
     * @brief notify bluetooth register callstatus callback
     * @return Returns sptr<ICallStatusCallback>.
     */
    sptr<ICallStatusCallback> RegisterBluetoothCallManagerCallbackPtr(std::string &macAddress) override;

    /**
     * @brief send ussd response to modem
     *
     * @param slotId[in] the slot id
     * @param content[in] the content need send to modem
     * @return Returns TELEPHONY_SUCCESS on success, others on failure.
     */
    int32_t SendUssdResponse(int32_t slotId, const std::string &content) override;

    /**
     * SetCallPolicyInfo
     *
     * @brief set telephony call trust/block list policy
     * @param isDialingTrustlist[in], dialing policy flag, false is block, true is trust.
     * @param dialingList[in], dialing trust/block number list.
     * @param isIncomingTrustlist[in], incoming policy flag, false is block, true is trust.
     * @param incomingList[in], incoming trust/block number list.
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallPolicyInfo(bool isDialingTrustlist, const std::vector<std::string> &dialingList,
        bool isIncomingTrustlist, const std::vector<std::string> &incomingList) override;
private:
    std::string GetBundleInfo();
    int32_t dealCeliaCallEvent(int32_t callId);
    int32_t HandleDisplaySpecifiedCallPage(int32_t callId);
    int32_t HandleCeliaAutoAnswerCall(int32_t callId, bool enable);
    int32_t HandleVoIPCallEvent(int32_t callId, std::string &eventName);
    void BtCallWaitSlotId(AppExecFwk::PacMap &dialInfo, const std::u16string &number);

private:
    enum ServiceRunningState {
        STATE_STOPPED = 0,
        STATE_RUNNING,
    };
    const std::string SOS_PULL_CALL_PAGE = "2";

    ServiceRunningState state_ { ServiceRunningState::STATE_STOPPED };
#ifndef TELEPHONY_VOIP_CALL_MANAGER_SYS_ABILITY_ID
#define TELEPHONY_VOIP_CALL_MANAGER_SYS_ABILITY_ID 65968
#endif
    std::shared_ptr<CallControlManager> callControlManagerPtr_;
    std::map<uint32_t, sptr<IRemoteObject>> proxyObjectPtrMap_;
    std::vector<std::string> supportSpecialCode_ { "*#2846#", "*#2846*",
        "*#*#2846579#*#*", "#1#", "*#28465#", "*#*#121314#*#*", "*#*#19467328#*#*",
        "*#*#2589#*#*", "*#*#2845#*#*", "*#*#2846#*#*", "*#*#2847#*#*", "*#*#28465#*#*",
        "*#*#2627#*#*", "*#*#258#*#*", "*#28460" };
    std::mutex lock_;
    std::mutex bluetoothCallCallbackLock_;
    const int32_t startTime_ = 1900;
    const int32_t extraMonth_ = 1;
    int64_t bindTime_ = 0L;
    int32_t spendTime_ = 0;
    sptr<ICallStatusCallback> bluetoothCallCallbackPtr_ = nullptr;
    std::shared_ptr<BluetoothCallState> bluetoothCallObserver_  = nullptr;
    std::shared_ptr<CallStatusManager> callStatusManagerPtr_ = nullptr;
};
} // namespace Telephony
} // namespace OHOS

#endif // CALL_MANAGER_SERVICE_H
