/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define private public
#define protected public
#include "audio_control_manager.h"
#include "bluetooth_call.h"
#include "call_state_processor.h"
#include "call_control_manager.h"
#include "common_event_manager.h"
#include "gtest/gtest.h"
#include "ims_call.h"
#include "voip_call.h"

namespace OHOS::Telephony {
using namespace testing::ext;
constexpr int WAIT_TIME = 5;
constexpr int VALID_CALL_ID = 1;
constexpr const char* NUMBER = "10086";
class ZeroBranch9Test : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase();
    static void TearDownTestCase(); 
};
void ZeroBranch9Test::SetUp()
{
    DelayedSingleton<AudioProxy>::GetInstance()->SetAudioMicStateChangeCallback();
    DelayedSingleton<AudioProxy>::GetInstance()->SetAudioDeviceChangeCallback();
    DelayedSingleton<AudioProxy>::GetInstance()->SetAudioPreferDeviceChangeCallback();
}

void ZeroBranch9Test::TearDown()
{
    DelayedSingleton<AudioProxy>::GetInstance()->UnsetAudioMicStateChangeCallback();
    DelayedSingleton<AudioProxy>::GetInstance()->UnsetDeviceChangeCallback();
    DelayedSingleton<AudioProxy>::GetInstance()->UnsetAudioPreferDeviceChangeCallback();
}

void ZeroBranch9Test::SetUpTestCase() {}
    
void ZeroBranch9Test::TearDownTestCase() {
    sleep(WAIT_TIME);
}

/**
 * @tc.number   Telephony_AudioControlManager_001
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(ZeroBranch9Test, Telephony_AudioControlManager_001, Function | MediumTest | Level3)
{
    DialParaInfo mDialParaInfo;
    mDialParaInfo.accountId = 0;
    sptr<OHOS::Telephony::CallBase> callObjectPtr = nullptr;
    DelayedSingleton<CallStateProcessor>::GetInstance()->holdingCalls_.insert(1);
    auto audioControl = DelayedSingleton<AudioControlManager>::GetInstance();
    audioControl->VideoStateUpdated(callObjectPtr, VideoStateType::TYPE_VOICE, VideoStateType::TYPE_VIDEO);
    callObjectPtr = new IMSCall(mDialParaInfo);
    callObjectPtr->SetCallType(CallType::TYPE_IMS);
    audioControl->VideoStateUpdated(callObjectPtr, VideoStateType::TYPE_VOICE, VideoStateType::TYPE_VIDEO);
    callObjectPtr->SetCrsType(2);
    audioControl->VideoStateUpdated(callObjectPtr, VideoStateType::TYPE_VOICE, VideoStateType::TYPE_VIDEO);
    audioControl->UpdateDeviceTypeForVideoOrSatelliteCall();
    audioControl->MuteNetWorkRingTone();
    audioControl->IsBtOrWireHeadPlugin();
    ASSERT_FALSE(audioControl->IsVideoCall(VideoStateType::TYPE_RECEIVE_ONLY));
    sptr<CallBase> call = nullptr;
    audioControl->IncomingCallHungUp(call, false, "");
    audioControl->CallStateUpdated(call, TelCallState::CALL_STATUS_DIALING, TelCallState::CALL_STATUS_ALERTING);
    call = new VoIPCall(mDialParaInfo);
    call->SetCallType(CallType::TYPE_VOIP);
    audioControl->CallStateUpdated(call, TelCallState::CALL_STATUS_DIALING, TelCallState::CALL_STATUS_ALERTING);
    audioControl->HandleCallStateUpdated(call, TelCallState::CALL_STATUS_DIALING, TelCallState::CALL_STATUS_ANSWERED);
    audioControl->HandleNextState(call, TelCallState::CALL_STATUS_ALERTING);
    audioControl->HandleNextState(call, TelCallState::CALL_STATUS_INCOMING);
    audioControl->HandleNextState(call, TelCallState::CALL_STATUS_WAITING);
    audioControl->HandlePriorState(call, TelCallState::CALL_STATUS_ALERTING);
    audioControl->HandlePriorState(call, TelCallState::CALL_STATUS_INCOMING);
    audioControl->HandlePriorState(call, TelCallState::CALL_STATUS_WAITING);
    audioControl->HandlePriorState(call, TelCallState::CALL_STATUS_HOLDING);
    call->SetCallRunningState(CallRunningState::CALL_RUNNING_STATE_ACTIVE);
    audioControl->HandlePriorState(call, TelCallState::CALL_STATUS_INCOMING);
    audioControl->HandlePriorState(call, TelCallState::CALL_STATUS_WAITING);
    audioControl->UpdateForegroundLiveCall();
    audioControl->ProcessAudioWhenCallActive(call);
    std::string emptyNumber = "";
    call->SetAccountNumber(emptyNumber);
    audioControl->HandleNewActiveCall(call);
    call->SetAccountNumber(NUMBER);
    call->SetCallType(CallType::TYPE_CS);
    audioControl->HandleNewActiveCall(call);
    call->SetCallType(CallType::TYPE_SATELLITE);
    audioControl->HandleNewActiveCall(call);
    call->SetCallType(CallType::TYPE_ERR_CALL);
    audioControl->HandleNewActiveCall(call);
    audioControl->GetCallList().empty();
    ASSERT_TRUE(audioControl->GetCurrentActiveCall() == nullptr);
}

/**
 * @tc.number   Telephony_AudioControlManager_002
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(ZeroBranch9Test, Telephony_AudioControlManager_002, Function | MediumTest | Level3)
{
    auto audioControl = DelayedSingleton<AudioControlManager>::GetInstance();
    AudioDevice device;
    device.deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PAD;
    if (memset_s(device.address, kMaxAddressLen + 1, 0, kMaxAddressLen + 1) != EOK) {
        return;
    }
    audioControl->SetAudioDevice(device);
    device.deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PHONE;
    audioControl->SetAudioDevice(device);
    device.deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_AUTOMOTIVE;
    audioControl->SetAudioDevice(device);
    device.deviceType = AudioDeviceType::DEVICE_BLUETOOTH_SCO;
    audioControl->SetAudioDevice(device);
    audioControl->PlaySoundtone();
    audioControl->GetInitAudioDeviceType();
    audioControl->SetMute(false);
    audioControl->MuteRinger();
    audioControl->PlayCallEndedTone(CallEndedType::PHONE_IS_BUSY);
    audioControl->PlayCallEndedTone(CallEndedType::CALL_ENDED_NORMALLY);
    audioControl->PlayCallEndedTone(CallEndedType::UNKNOWN);
    audioControl->PlayCallEndedTone(CallEndedType::INVALID_NUMBER);
    audioControl->PlayCallEndedTone(static_cast<CallEndedType>(5));
    audioControl->GetCallBase(VALID_CALL_ID);
    audioControl->IsEmergencyCallExists();
    audioControl->SetToneState(ToneState::TONEING);
    audioControl->IsNumberAllowed(NUMBER);
    audioControl->audioInterruptState_ = AudioInterruptState::INTERRUPT_STATE_ACTIVATED;
    ASSERT_TRUE(audioControl->IsAudioActivated());
}

/**
 * @tc.number   Telephony_AudioControlManager_003
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(ZeroBranch9Test, Telephony_AudioControlManager_003, Function | MediumTest | Level3)
{
    auto audioControl = DelayedSingleton<AudioControlManager>::GetInstance();
    audioControl->IsTonePlaying();
    audioControl->IsCurrentRinging();
    audioControl->PlayRingback();
    audioControl->StopWaitingTone();
    audioControl->PlayDtmfTone('a');
    audioControl->StopDtmfTone();
    audioControl->OnPostDialNextChar('a');
    audioControl->IsSoundPlaying();
    ASSERT_TRUE(audioControl->IsVideoCall(VideoStateType::TYPE_VIDEO));
}

/**
 * @tc.number   Telephony_AudioControlManager_004
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(ZeroBranch9Test, Telephony_AudioControlManager_004, Function | MediumTest | Level3)
{
    auto audioControl = DelayedSingleton<AudioControlManager>::GetInstance();
    audioControl->isCrsVibrating_ = true;
    audioControl->MuteNetWorkRingTone();
    DisconnectedDetails details;
    audioControl->CallDestroyed(details);
    audioControl->toneState_ = ToneState::CALLENDED;
    audioControl->StopCallTone();
    audioControl->tone_ = nullptr;
    audioControl->StopCallTone();
    audioControl->GetAudioInterruptState();
    audioControl->SetVolumeAudible();
    audioControl->ringState_ = RingState::RINGING;
    audioControl->MuteRinger();
    ASSERT_TRUE(audioControl->GetCurrentActiveCall() == nullptr);
}

/**
 * @tc.number   Telephony_AudioControlManager_005
 * @tc.name     test error branch
 * @tc.desc     Function test
 */
HWTEST_F(ZeroBranch9Test, Telephony_AudioControlManager_005, Function | MediumTest | Level3)
{
    auto audioControl = DelayedSingleton<AudioControlManager>::GetInstance();
    audioControl->ExcludeBluetoothSco();
    audioControl->UnexcludeBluetoothSco();
    audioControl->UpdateDeviceTypeForCrs();
    EXPECT_FALSE(audioControl->IsRingingVibrateModeOn());
    auto callControl = DelayedSingleton<CallControlManager>::GetInstance();
    callControl->SetVoIPCallState(1);
    EXPECT_TRUE(audioControl->IsVoIPCallActived());
    callControl->SetVoIPCallState(0);
    EXPECT_FALSE(audioControl->IsVoIPCallActived());
    callControl->SetVoIPCallState(-1);
    EXPECT_FALSE(audioControl->IsVoIPCallActived());
    CallObjectManager::callObjectPtrList_.clear();
    EXPECT_FALSE(audioControl->IsBtCallDisconnected());
    DialParaInfo info;
    sptr<CallBase> call = new BluetoothCall(info, "");
    CallObjectManager::AddOneCallObject(call);
    call->SetCallRunningState(CallRunningState::CALL_RUNNING_STATE_RINGING);
    EXPECT_FALSE(audioControl->IsBtCallDisconnected());
    call->SetCallRunningState(CallRunningState::CALL_RUNNING_STATE_ENDED);
    EXPECT_FALSE(audioControl->IsBtCallDisconnected());
    call->SetCallType(CallType::TYPE_BLUETOOTH);
    EXPECT_TRUE(audioControl->IsBtCallDisconnected());
    audioControl->SetRingToneVolume(0.0f);
    audioControl->PlaySoundtone();
    audioControl->SetRingToneVolume(0.0f);
    audioControl->SetRingToneVolume(0.5f);
    audioControl->SetRingToneVolume(1.5f);
    audioControl->StopSoundtone();
    audioControl->ringState_ = RingState::RINGING;
    audioControl->StopRingtone();
    audioControl->ring_ = nullptr;
    audioControl->StopRingtone();
    audioControl->PlayCallTone(ToneDescriptor::TONE_ENGAGED);
    EXPECT_EQ(audioControl->StopRingback(), 0);
}
}