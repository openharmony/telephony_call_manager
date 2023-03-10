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

#include "audio_device_manager.h"

#include "audio_control_manager.h"
#include "bluetooth_call_manager.h"
#include "bluetooth_device_state.h"
#include "call_ability_report_proxy.h"
#include "earpiece_device_state.h"
#include "inactive_device_state.h"
#include "speaker_device_state.h"
#include "telephony_log_wrapper.h"
#include "wired_headset_device_state.h"

namespace OHOS {
namespace Telephony {
bool AudioDeviceManager::isBtScoDevEnable_ = false;
bool AudioDeviceManager::isSpeakerAvailable_ = true; // default available
bool AudioDeviceManager::isEarpieceAvailable_ = false;
bool AudioDeviceManager::isWiredHeadsetConnected_ = false;
bool AudioDeviceManager::isBtScoConnected_ = false;

AudioDeviceManager::AudioDeviceManager()
    : audioDeviceType_(AudioDeviceType::DEVICE_UNKNOWN), currentAudioDevice_(nullptr), isAudioActivated_(false)
{}

AudioDeviceManager::~AudioDeviceManager()
{
    memberFuncMap_.clear();
}

void AudioDeviceManager::Init()
{
    memberFuncMap_[AudioEvent::ENABLE_DEVICE_EARPIECE] = &AudioDeviceManager::EnableEarpiece;
    memberFuncMap_[AudioEvent::ENABLE_DEVICE_SPEAKER] = &AudioDeviceManager::EnableSpeaker;
    memberFuncMap_[AudioEvent::ENABLE_DEVICE_WIRED_HEADSET] = &AudioDeviceManager::EnableWiredHeadset;
    memberFuncMap_[AudioEvent::ENABLE_DEVICE_BLUETOOTH] = &AudioDeviceManager::EnableBtSco;
    currentAudioDevice_ = std::make_unique<InactiveDeviceState>();
    if (currentAudioDevice_ == nullptr) {
        TELEPHONY_LOGE("current audio device nullptr");
    }
    if (memset_s(&info_, sizeof(AudioDeviceInfo), 0, sizeof(AudioDeviceInfo)) != EOK) {
        TELEPHONY_LOGE("memset_s address fail");
        return;
    }
    AudioDevice speaker = {
        .deviceType = AudioDeviceType::DEVICE_SPEAKER,
        .address = { 0 },
    };
    info_.audioDeviceList.push_back(speaker);
#ifdef DEVICE_EARPIECE_SUPPORT
    AudioDevice earpiece = {
        .deviceType = AudioDeviceType::DEVICE_EARPIECE,
        .address = { 0 },
    };
    info_.audioDeviceList.push_back(earpiece);
#endif
}

void AudioDeviceManager::AddAudioDeviceList(const std::string &address, AudioDeviceType deviceType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AudioDevice>::iterator it = info_.audioDeviceList.begin();
    while (it != info_.audioDeviceList.end()) {
        if (it->address == address && it->deviceType == deviceType) {
            TELEPHONY_LOGI("device is already existenced");
            return;
        }
        if (deviceType == AudioDeviceType::DEVICE_WIRED_HEADSET && it->deviceType == AudioDeviceType::DEVICE_EARPIECE) {
            it = info_.audioDeviceList.erase(it);
            TELEPHONY_LOGI("remove Earpiece device success");
        } else {
            it++;
        }
    }
    AudioDevice audioDevice;
    if (memset_s(&audioDevice, sizeof(AudioDevice), 0, sizeof(AudioDevice)) != EOK) {
        TELEPHONY_LOGE("memset_s fail");
        return;
    }
    audioDevice.deviceType = deviceType;
    if (address.length() > kMaxAddressLen) {
        TELEPHONY_LOGE("address is not too long");
        return;
    }
    if (memcpy_s(audioDevice.address, kMaxAddressLen, address.c_str(), address.length()) != EOK) {
        TELEPHONY_LOGE("memcpy_s address fail");
        return;
    }
    info_.audioDeviceList.push_back(audioDevice);
    if (deviceType == AudioDeviceType::DEVICE_WIRED_HEADSET) {
        SetDeviceAvailable(AudioDeviceType::DEVICE_WIRED_HEADSET, true);
    }
    if (deviceType == AudioDeviceType::DEVICE_BLUETOOTH_SCO) {
        SetDeviceAvailable(AudioDeviceType::DEVICE_BLUETOOTH_SCO, true);
    }
    TELEPHONY_LOGI("AddAudioDeviceList success");
}

void AudioDeviceManager::RemoveAudioDeviceList(const std::string &address, AudioDeviceType deviceType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool needAddEarpiece = true;
    std::vector<AudioDevice>::iterator it = info_.audioDeviceList.begin();
    while (it != info_.audioDeviceList.end()) {
        if (it->deviceType == AudioDeviceType::DEVICE_EARPIECE) {
            needAddEarpiece = false;
        }
        if (it->address == address && it->deviceType == deviceType) {
            it = info_.audioDeviceList.erase(it);
        } else {
            it++;
        }
    }

    bool wiredHeadsetExist = false;
    bool blueToothScoExist = false;
    for (auto &elem : info_.audioDeviceList) {
        if (elem.deviceType == AudioDeviceType::DEVICE_WIRED_HEADSET) {
            wiredHeadsetExist = true;
        }
        if (elem.deviceType == AudioDeviceType::DEVICE_BLUETOOTH_SCO) {
            blueToothScoExist = true;
        }
    }
    if (deviceType == AudioDeviceType::DEVICE_WIRED_HEADSET && !wiredHeadsetExist) {
        SetDeviceAvailable(AudioDeviceType::DEVICE_WIRED_HEADSET, false);
    }
    if (deviceType == AudioDeviceType::DEVICE_BLUETOOTH_SCO && !blueToothScoExist) {
        SetDeviceAvailable(AudioDeviceType::DEVICE_BLUETOOTH_SCO, false);
    }
#ifdef DEVICE_EARPIECE_SUPPORT
    if (needAddEarpiece && deviceType == AudioDeviceType::DEVICE_WIRED_HEADSET && !wiredHeadsetExist) {
        AudioDevice audioDevice = {
            .deviceType = AudioDeviceType::DEVICE_EARPIECE,
            .address = { 0 },
        };
        info_.audioDeviceList.push_back(audioDevice);
        TELEPHONY_LOGI("add Earpiece device success");
    }
#endif
    TELEPHONY_LOGI("RemoveAudioDeviceList success");
}

bool AudioDeviceManager::InitAudioDevice()
{
    // when audio deactivate interrupt , reinit
    // when external audio device connection state changed , reinit
    auto device = DelayedSingleton<AudioControlManager>::GetInstance()->GetInitAudioDeviceType();
    return SwitchDevice(device);
}

bool AudioDeviceManager::ProcessEvent(AudioEvent event)
{
    if (currentAudioDevice_ == nullptr) {
        TELEPHONY_LOGE("current audio device nullptr");
        return false;
    }
    bool result = false;
    switch (event) {
        case AudioEvent::ENABLE_DEVICE_EARPIECE:
        case AudioEvent::ENABLE_DEVICE_SPEAKER:
        case AudioEvent::ENABLE_DEVICE_BLUETOOTH:
        case AudioEvent::ENABLE_DEVICE_WIRED_HEADSET:
            result = SwitchDevice(event);
            break;
        case AudioEvent::AUDIO_ACTIVATED:
        case AudioEvent::AUDIO_RINGING:
            if (!isAudioActivated_) {
                isAudioActivated_ = true;
                std::shared_ptr<BluetoothCallManager> bluetoothCallManager = std::make_shared<BluetoothCallManager>();
                // Gets whether the device can be started from the configuration
                if (bluetoothCallManager->IsBtAvailble()) {
                    return DelayedSingleton<BluetoothConnection>::GetInstance()->ConnectBtSco();
                }
                result = currentAudioDevice_->ProcessEvent(event);
            }
            break;
        case AudioEvent::AUDIO_DEACTIVATED:
            if (isAudioActivated_) {
                isAudioActivated_ = false;
                result = InitAudioDevice();
            }
            break;
        case AudioEvent::BLUETOOTH_SCO_CONNECTED:
            isBtScoConnected_ = true;
            currentAudioDevice_ = std::make_unique<BluetoothDeviceState>();
            if (currentAudioDevice_ == nullptr) {
                return false;
            }
            result = currentAudioDevice_->ProcessEvent(event);
            break;
        case AudioEvent::BLUETOOTH_SCO_DISCONNECTED:
            isBtScoConnected_ = false;
            result = currentAudioDevice_->ProcessEvent(event);
            break;
        case AudioEvent::INIT_AUDIO_DEVICE:
            result = InitAudioDevice();
            break;
        default:
            break;
    }
    return result;
}

bool AudioDeviceManager::SwitchDevice(AudioEvent event)
{
    auto itFunc = memberFuncMap_.find(event);
    if (itFunc != memberFuncMap_.end() && itFunc->second != nullptr) {
        auto memberFunc = itFunc->second;
        return (this->*memberFunc)();
    }
    return false;
}

bool AudioDeviceManager::ConnectBtScoWithAddress(const std::string &bluetoothAddress)
{
    std::shared_ptr<BluetoothCallManager> bluetoothCallManager = std::make_shared<BluetoothCallManager>();
    if (bluetoothCallManager->ConnectBtSco(bluetoothAddress)) {
        return true;
    }
    return false;
}

bool AudioDeviceManager::SwitchDevice(AudioDeviceType device)
{
    bool result = false;
    std::lock_guard<std::mutex> lock(mutex_);
    switch (device) {
        case AudioDeviceType::DEVICE_EARPIECE:
            result = EnableEarpiece();
            break;
        case AudioDeviceType::DEVICE_SPEAKER:
            result = EnableSpeaker();
            break;
        case AudioDeviceType::DEVICE_WIRED_HEADSET:
            result = EnableWiredHeadset();
            break;
        case AudioDeviceType::DEVICE_BLUETOOTH_SCO:
            result = EnableBtSco();
            break;
        case AudioDeviceType::DEVICE_DISABLE:
            result = DisableAll();
            break;
        default:
            break;
    }
    TELEPHONY_LOGI("switch device lock release");
    return result;
}

bool AudioDeviceManager::EnableSpeaker()
{
    if (isSpeakerAvailable_ && DelayedSingleton<AudioProxy>::GetInstance()->SetSpeakerDevActive()) {
        currentAudioDevice_ = std::make_unique<SpeakerDeviceState>();
        if (currentAudioDevice_ == nullptr) {
            TELEPHONY_LOGE("make_unique SpeakerDeviceState failed");
            return false;
        }
        TELEPHONY_LOGI("speaker enabled , current audio device : speaker");
        SetCurrentAudioDevice(AudioDeviceType::DEVICE_SPEAKER);
        return true;
    }
    TELEPHONY_LOGI("enable speaker device failed");
    return false;
}

bool AudioDeviceManager::EnableEarpiece()
{
    if (isEarpieceAvailable_ && DelayedSingleton<AudioProxy>::GetInstance()->SetEarpieceDevActive()) {
        currentAudioDevice_ = std::make_unique<EarpieceDeviceState>();
        if (currentAudioDevice_ == nullptr) {
            TELEPHONY_LOGE("make_unique EarpieceDeviceState failed");
            return false;
        }
        TELEPHONY_LOGI("earpiece enabled , current audio device : earpiece");
        SetCurrentAudioDevice(AudioDeviceType::DEVICE_EARPIECE);
        return true;
    }
    TELEPHONY_LOGI("enable earpiece device failed");
    return false;
}

bool AudioDeviceManager::EnableWiredHeadset()
{
    if (isWiredHeadsetConnected_ && DelayedSingleton<AudioProxy>::GetInstance()->SetWiredHeadsetDevActive()) {
        currentAudioDevice_ = std::make_unique<WiredHeadsetDeviceState>();
        if (currentAudioDevice_ == nullptr) {
            TELEPHONY_LOGE("make_unique WiredHeadsetDeviceState failed");
            return false;
        }
        TELEPHONY_LOGI("wired headset enabled , current audio device : wired headset");
        SetCurrentAudioDevice(AudioDeviceType::DEVICE_WIRED_HEADSET);
        return true;
    }
    TELEPHONY_LOGI("enable wired headset device failed");
    return false;
}

bool AudioDeviceManager::EnableBtSco()
{
    if (isBtScoConnected_ && DelayedSingleton<AudioProxy>::GetInstance()->SetBluetoothDevActive()) {
        currentAudioDevice_ = std::make_unique<BluetoothDeviceState>();
        if (currentAudioDevice_ == nullptr) {
            TELEPHONY_LOGE("make_unique BluetoothDeviceState failed");
            return false;
        }
        TELEPHONY_LOGI("bluetooth sco enabled , current audio device : bluetooth sco");
        SetCurrentAudioDevice(AudioDeviceType::DEVICE_BLUETOOTH_SCO);
        return true;
    }
    TELEPHONY_LOGI("enable bluetooth sco device failed");
    return false;
}

bool AudioDeviceManager::DisableAll()
{
    isBtScoDevEnable_ = false;
    isWiredHeadsetDevEnable_ = false;
    isSpeakerDevEnable_ = false;
    isEarpieceDevEnable_ = false;
    currentAudioDevice_ = std::make_unique<InactiveDeviceState>();
    if (currentAudioDevice_ == nullptr) {
        TELEPHONY_LOGE("make_unique InactiveDeviceState failed");
        return false;
    }
    TELEPHONY_LOGI("current audio device : all audio devices disabled");
    return true;
}

void AudioDeviceManager::SetCurrentAudioDevice(AudioDeviceType deviceType)
{
    if (audioDeviceType_ == AudioDeviceType::DEVICE_BLUETOOTH_SCO && audioDeviceType_ != deviceType) {
        DelayedSingleton<BluetoothConnection>::GetInstance()->SetBtScoState(SCO_STATE_DISCONNECTED);
    } else if (audioDeviceType_ != AudioDeviceType::DEVICE_BLUETOOTH_SCO &&
               deviceType == AudioDeviceType::DEVICE_BLUETOOTH_SCO) {
        DelayedSingleton<BluetoothConnection>::GetInstance()->SetBtScoState(SCO_STATE_CONNECTED);
    }
    audioDeviceType_ = deviceType;
    ReportAudioDeviceChange();
}

int32_t AudioDeviceManager::ReportAudioDeviceChange()
{
    info_.currentAudioDevice.deviceType = audioDeviceType_;
    std::string address = "";
    if (audioDeviceType_ == AudioDeviceType::DEVICE_BLUETOOTH_SCO) {
        std::shared_ptr<BluetoothCallManager> bluetoothCallManager = std::make_shared<BluetoothCallManager>();
        address = bluetoothCallManager->GetConnectedScoAddr();
    }
    if (address.length() > kMaxAddressLen) {
        TELEPHONY_LOGE("address is not too long");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (memcpy_s(info_.currentAudioDevice.address, kMaxAddressLen, address.c_str(), address.length()) != EOK) {
        TELEPHONY_LOGE("memcpy_s address fail");
        return TELEPHONY_ERR_MEMCPY_FAIL;
    }
    info_.isMuted = DelayedSingleton<AudioProxy>::GetInstance()->IsMicrophoneMute();
    return DelayedSingleton<CallAbilityReportProxy>::GetInstance()->ReportAudioDeviceChange(info_);
}

AudioDeviceType AudioDeviceManager::GetCurrentAudioDevice()
{
    return audioDeviceType_;
}

bool AudioDeviceManager::IsEarpieceDevEnable()
{
    return isEarpieceDevEnable_;
}

bool AudioDeviceManager::IsWiredHeadsetDevEnable()
{
    return isWiredHeadsetDevEnable_;
}

bool AudioDeviceManager::IsSpeakerDevEnable()
{
    return isSpeakerDevEnable_;
}

bool AudioDeviceManager::IsBtScoDevEnable()
{
    return isBtScoDevEnable_;
}

bool AudioDeviceManager::IsBtScoConnected()
{
    return isBtScoConnected_;
}

bool AudioDeviceManager::IsWiredHeadsetConnected()
{
    return isWiredHeadsetConnected_;
}

bool AudioDeviceManager::IsEarpieceAvailable()
{
    return isEarpieceAvailable_;
}

bool AudioDeviceManager::IsSpeakerAvailable()
{
    return isSpeakerAvailable_;
}

void AudioDeviceManager::SetDeviceAvailable(AudioDeviceType deviceType, bool available)
{
    switch (deviceType) {
        case AudioDeviceType::DEVICE_SPEAKER:
            isSpeakerAvailable_ = available;
            break;
        case AudioDeviceType::DEVICE_EARPIECE:
            isEarpieceAvailable_ = available;
            break;
        case AudioDeviceType::DEVICE_BLUETOOTH_SCO:
            isBtScoConnected_ = available;
            break;
        case AudioDeviceType::DEVICE_WIRED_HEADSET:
            isWiredHeadsetConnected_ = available;
            break;
        default:
            break;
    }
}
} // namespace Telephony
} // namespace OHOS