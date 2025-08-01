/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#include <thread>
#include <pthread.h>

#include "distributed_call_manager.h"
#include "audio_control_manager.h"
#include "call_object_manager.h"
#include "telephony_log_wrapper.h"
#include "nlohmann/json.hpp"
#include "i_distributed_device_callback.h"
#ifdef ABILITY_BLUETOOTH_SUPPORT
#include "bluetooth_device.h"
#endif

using json = nlohmann::json;

namespace OHOS {
namespace Telephony {
using namespace AudioStandard;
namespace {
const size_t INT32_MIN_ID_LENGTH = 3;
const size_t INT32_SHORT_ID_LENGTH = 20;
const size_t INT32_PLAINTEXT_LENGTH = 4;
const int32_t DCALL_SWITCH_DEVICE_TYPE_SOURCE = 0;
const int32_t DCALL_SWITCH_DEVICE_TYPE_SINK = 1;
const int32_t DISTRIBUTED_CALL_SOURCE_SA_ID = 9855;
const int32_t IS_CELIA_CALL = 1;
const int32_t DEFAULT_DCALL_HFP_FLAG_VALUE = -1;
const int32_t DCALL_BT_HEADSET_UNWEAR_ACTION = 1;
const std::string CALLBACK_NAME = "telephony";
const std::string DISTRIBUTED_AUDIO_DEV_CAR = "dCar";
const std::string DISTRIBUTED_AUDIO_DEV_PHONE = "dPhone";
const std::string DISTRIBUTED_AUDIO_DEV_PAD = "dPad";
const std::string SWITCH_ON_DCALL_THREAD_NAME = "switch on dcall";

std::string GetAnonyString(const std::string &value)
{
    std::string res;
    std::string tmpStr("******");
    size_t strLen = value.length();
    if (strLen < INT32_MIN_ID_LENGTH) {
        return tmpStr;
    }
    if (strLen <= INT32_SHORT_ID_LENGTH) {
        res += value[0];
        res += tmpStr;
        res += value[strLen - 1];
    } else {
        res.append(value, 0, INT32_PLAINTEXT_LENGTH);
        res += tmpStr;
        res.append(value, strLen - INT32_PLAINTEXT_LENGTH, INT32_PLAINTEXT_LENGTH);
    }
    return res;
}

bool IsDistributedAudioDevice(const AudioDevice& device)
{
    if ((device.deviceType == AudioDeviceType::DEVICE_DISTRIBUTED_PHONE) ||
        (device.deviceType == AudioDeviceType::DEVICE_DISTRIBUTED_PAD) ||
        (device.deviceType == AudioDeviceType::DEVICE_DISTRIBUTED_AUTOMOTIVE)) {
        return true;
    }
    return false;
}
}

DistributedCallManager::DistributedCallManager()
{
    TELEPHONY_LOGI("DistributedCallManager constructed.");
}

DistributedCallManager::~DistributedCallManager()
{
    TELEPHONY_LOGI("DistributedCallManager destructed.");
}

void DistributedCallManager::Init()
{
    TELEPHONY_LOGI("Init start.");
    statusChangeListener_ = new (std::nothrow) DCallSystemAbilityListener();
    if (statusChangeListener_ == nullptr) {
        TELEPHONY_LOGE("failed to create statusChangeListener");
        return;
    }
    auto managerPtr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (managerPtr == nullptr) {
        TELEPHONY_LOGE("get system ability manager error");
        return;
    }
    int32_t ret = managerPtr->SubscribeSystemAbility(DISTRIBUTED_CALL_SOURCE_SA_ID, statusChangeListener_);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("failed to subscribe dcall service SA: %{public}d", DISTRIBUTED_CALL_SOURCE_SA_ID);
        return;
    }
    TELEPHONY_LOGI("Init end.");
}

bool DistributedCallManager::CreateDAudioDevice(const std::string& devId, AudioDevice& device)
{
    if (!devId.length()) {
        TELEPHONY_LOGE("dcall devId is invalid");
        return false;
    }
    OHOS::DistributedHardware::DCallDeviceInfo devInfo;
    {
        std::lock_guard<std::mutex> lock(dcallProxyMtx_);
        if (dcallProxy_ == nullptr) {
            TELEPHONY_LOGE("dcallProxy_ is nullptr");
            return false;
        }
        int32_t ret = dcallProxy_->GetDCallDeviceInfo(devId, devInfo);
        if (ret != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGI("get dcall device info failed.");
            return false;
        }
    }
    std::string devTypeName;
    std::string devName = devInfo.devName;
    if (devInfo.devType == OHOS::DistributedHardware::DCallDeviceType::DISTRIBUTED_DEVICE_PHONE) {
        devTypeName = DISTRIBUTED_AUDIO_DEV_PHONE;
        device.deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PHONE;
    } else if (devInfo.devType == OHOS::DistributedHardware::DCallDeviceType::DISTRIBUTED_DEVICE_PAD) {
        devTypeName = DISTRIBUTED_AUDIO_DEV_PAD;
        device.deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PAD;
    } else {
        devTypeName = DISTRIBUTED_AUDIO_DEV_CAR;
        device.deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_AUTOMOTIVE;
    }
    json addressJson;
    addressJson["devName"] = devName;
    addressJson["devId"] = devId;
    std::string addressStr = addressJson.dump();
    if (memcpy_s(device.address, kMaxAddressLen, addressStr.c_str(), addressStr.length()) != EOK) {
        TELEPHONY_LOGE("memcpy_s failed.");
        return false;
    }
    TELEPHONY_LOGI("create distributed audio device succeed.");
    return true;
}

std::string DistributedCallManager::GetDevIdFromAudioDevice(const AudioDevice& device)
{
    std::string devId = "";
    if (!IsDistributedAudioDevice(device)) {
        TELEPHONY_LOGE("not distributed audio device, device type: %{public}d", device.deviceType);
        return devId;
    }
    std::string address = device.address;
    if (!address.length()) {
        TELEPHONY_LOGE("invalid address");
        return devId;
    }
    json addressJson = json::parse(address, nullptr, false);
    if (addressJson.is_null() || addressJson.is_discarded()) {
        TELEPHONY_LOGE("json value is null or discarded.");
        return devId;
    }
    if (!addressJson.contains("devId")) {
        TELEPHONY_LOGE("json not contain devId.");
        return devId;
    }
    if (!addressJson["devId"].is_string()) {
        TELEPHONY_LOGE("json has no devId string.");
        return devId;
    }
    devId = addressJson["devId"];
    TELEPHONY_LOGI("devId: %{public}s", GetAnonyString(devId).c_str());
    return devId;
}

int32_t DistributedCallManager::AddDCallDevice(const std::string& devId)
{
    TELEPHONY_LOGI("add dcall device, devId: %{public}s.", GetAnonyString(devId).c_str());
#ifdef ABILITY_BLUETOOTH_SUPPORT
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (dcallHfpListener_ == nullptr) {
            dcallHfpListener_ = std::make_shared<DCallHfpListener>();
            Bluetooth::HandsFreeAudioGateway::GetProfile()->RegisterObserver(dcallHfpListener_);
        }
    }
#endif
    std::lock_guard<std::mutex> lock(onlineDeviceMtx_);

    auto iter = onlineDCallDevices_.find(devId);
    if (iter != onlineDCallDevices_.end()) {
        TELEPHONY_LOGW("device is already exist, devId: %{public}s", GetAnonyString(devId).c_str());
        return TELEPHONY_SUCCESS;
    }

    AudioDevice device;
    if (!CreateDAudioDevice(devId, device)) {
        TELEPHONY_LOGE("failed to create distributed audio device, devId: %{public}s", GetAnonyString(devId).c_str());
        return TELEPHONY_ERR_FAIL;
    }

    DelayedSingleton<AudioDeviceManager>::GetInstance()->AddAudioDeviceList(device.address, device.deviceType, "");
    onlineDCallDevices_.emplace(devId, device);

    if (!dCallDeviceSwitchedOn_.load() && isCallActived_.load()) {
        if (device.deviceType == AudioDeviceType::DEVICE_DISTRIBUTED_AUTOMOTIVE && IsSelectVirtualModem()) {
            TELEPHONY_LOGI("switch call to auto motive as it is online");
            SwitchOnDCallDeviceAsync(device);
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t DistributedCallManager::RemoveDCallDevice(const std::string& devId)
{
    TELEPHONY_LOGI("remove dcall device, devId: %{public}s.", GetAnonyString(devId).c_str());
    std::lock_guard<std::mutex> lock(onlineDeviceMtx_);
    auto iter = onlineDCallDevices_.find(devId);
    if (iter != onlineDCallDevices_.end()) {
        std::string devId = GetDevIdFromAudioDevice(iter->second);
        std::string curDevId = GetConnectedDCallDeviceId();
        TELEPHONY_LOGI("removed devId: %{public}s, current devId: %{public}s",
            GetAnonyString(devId).c_str(), GetAnonyString(curDevId).c_str());
        DelayedSingleton<AudioDeviceManager>::GetInstance()->RemoveAudioDeviceList(
            iter->second.address, iter->second.deviceType);
        onlineDCallDevices_.erase(iter);
        if (curDevId == devId) {
            TELEPHONY_LOGI("current dcall device is removed, now reinit audio device.");
            dCallDeviceSwitchedOn_.store(false);
            ClearConnectedDCallDevice();
            DelayedSingleton<AudioDeviceManager>::GetInstance()->InitAudioDevice();
        }
    }
    return TELEPHONY_SUCCESS;
}

void DistributedCallManager::ClearDCallDevices()
{
    TELEPHONY_LOGI("clear dcall device.");
    std::lock_guard<std::mutex> lock(onlineDeviceMtx_);
    onlineDCallDevices_.clear();
}

void DistributedCallManager::NotifyOnlineDCallDevices(std::vector<std::string> devices)
{
    TELEPHONY_LOGI("notify online dcall devices start, size: %{public}d", static_cast<int32_t>(devices.size()));
    for (auto item : devices) {
        AddDCallDevice(item);
        TELEPHONY_LOGI("notify dcall device, devId: %{public}s", GetAnonyString(item).c_str());
    }
    TELEPHONY_LOGI("notify online dcall devices end.");
}

std::string DistributedCallManager::GetConnectedDCallDeviceAddr()
{
    std::lock_guard<std::mutex> lock(connectedDevMtx_);
    std::string addr = connectedAudioDevice_.address;
    return addr;
}

AudioDeviceType DistributedCallManager::GetConnectedDCallDeviceType()
{
    std::lock_guard<std::mutex> lock(connectedDevMtx_);
    AudioDeviceType type = connectedAudioDevice_.deviceType;
    return type;
}

std::string DistributedCallManager::GetConnectedDCallDeviceId()
{
    std::lock_guard<std::mutex> lock(connectedDevMtx_);
    std::string devId = "";
    if (dCallDeviceSwitchedOn_.load()) {
        devId = GetDevIdFromAudioDevice(connectedAudioDevice_);
    }
    return devId;
}

void DistributedCallManager::GetConnectedDCallDevice(AudioDevice& device)
{
    std::lock_guard<std::mutex> lock(connectedDevMtx_);
    device.deviceType = connectedAudioDevice_.deviceType;
    if (memcpy_s(device.address, kMaxAddressLen, connectedAudioDevice_.address, kMaxAddressLen) != EOK) {
        TELEPHONY_LOGE("memcpy_s failed.");
    }
}

void DistributedCallManager::SetConnectedDCallDevice(const AudioDevice& device)
{
    std::lock_guard<std::mutex> lock(connectedDevMtx_);
    connectedAudioDevice_.deviceType = device.deviceType;
    if (memcpy_s(connectedAudioDevice_.address, kMaxAddressLen, device.address, kMaxAddressLen) != EOK) {
        TELEPHONY_LOGE("memcpy_s failed.");
    }
}

void DistributedCallManager::ClearConnectedDCallDevice()
{
    std::lock_guard<std::mutex> lock(connectedDevMtx_);
    connectedAudioDevice_.deviceType = AudioDeviceType::DEVICE_UNKNOWN;
    if (memset_s(connectedAudioDevice_.address, kMaxAddressLen, 0, kMaxAddressLen) != EOK) {
        TELEPHONY_LOGE("memset_s failed.");
    }
}

bool DistributedCallManager::SwitchOnDCallDeviceSync(const AudioDevice& device)
{
    TELEPHONY_LOGI("switch dcall device on sync");
    if (!isCallActived_.load()) {
        TELEPHONY_LOGW("call is not active, no need switch");
        return true;
    }
    if (!IsDistributedAudioDevice(device)) {
        TELEPHONY_LOGE("not distributed audio device, device type: %{public}d", device.deviceType);
        return false;
    }
    std::string devId = GetDevIdFromAudioDevice(device);
    if (!devId.length()) {
        TELEPHONY_LOGE("dcall devId is invalid");
        return false;
    }
    TELEPHONY_LOGI("switch dcall device on start, devId: %{public}s", GetAnonyString(devId).c_str());
    int32_t ret;
    {
        std::lock_guard<std::mutex> lock(dcallProxyMtx_);
        if (dcallProxy_ == nullptr) {
            TELEPHONY_LOGE("dcallProxy_ is nullptr");
            return false;
        }
        if (isSwitching_) {
            TELEPHONY_LOGI("in switching");
            return false;
        }
        isSwitching_ = true;
        ReportDistributedDeviceInfo(device);
        ret = dcallProxy_->SwitchDevice(devId, DCALL_SWITCH_DEVICE_TYPE_SINK);
        isSwitching_ = false;
    }
    if (ret == TELEPHONY_SUCCESS) {
        dCallDeviceSwitchedOn_.store(true);
        SetConnectedDCallDevice(device);
        DelayedSingleton<AudioDeviceManager>::GetInstance()->SetCurrentAudioDevice(device.deviceType);
        TELEPHONY_LOGI("switch dcall device on succeed.");
        return true;
    }
    TELEPHONY_LOGI("switch dcall device on failed, ret: %{public}d.", ret);
    return false;
}

void DistributedCallManager::SetCallState(bool isActive)
{
    isCallActived_.store(isActive);
}

void DistributedCallManager::DealDisconnectCall()
{
    dCallDeviceSwitchedOn_.store(false);
    ClearConnectedDCallDevice();
}

bool DistributedCallManager::IsDistributedCarDeviceOnline()
{
    if (onlineDCallDevices_.size() > 0) {
        return true;
    }
    return false;
}

bool DistributedCallManager::isCeliaCall()
{
    sptr<CallBase> foregroundCall = CallObjectManager::GetForegroundCall(false);
    if (foregroundCall == nullptr) {
        TELEPHONY_LOGE("foregroundCall is nullptr!");
        return false;
    }
    int32_t celiaCallType = foregroundCall->GetCeliaCallType();
    if (celiaCallType == IS_CELIA_CALL) {
        TELEPHONY_LOGI("current is celia call, no need switch on dcall device.");
        return true;
    }
    return false;
}

void DistributedCallManager::SwitchOnDCallDeviceAsync(const AudioDevice& device)
{
    if (isCeliaCall()) {
        return;
    }
    auto weak = weak_from_this();
    TELEPHONY_LOGI("switch dcall device on async");
    std::thread switchThread = std::thread([weak, device]() {
        auto strong = weak.lock();
        if (strong) {
            strong->SwitchOnDCallDeviceSync(device);
            if (strong->isCeliaCall()) {
                strong->SwitchOffDCallDeviceSync();
                strong->ReportDistributedDeviceInfoForSwitchOff();
            }
        }
    });
    pthread_setname_np(switchThread.native_handle(), SWITCH_ON_DCALL_THREAD_NAME.c_str());
    switchThread.detach();
}

void DistributedCallManager::SwitchOffDCallDeviceSync()
{
    TELEPHONY_LOGI("switch dcall device off sync");
    if (!dCallDeviceSwitchedOn_.load()) {
        TELEPHONY_LOGE("distributed audio device not connected.");
        return;
    }
    std::string devId = GetConnectedDCallDeviceId();
    if (!devId.length()) {
        TELEPHONY_LOGE("dcall devId is invalid");
        return;
    }
    TELEPHONY_LOGI("switch dcall device off start, devId: %{public}s", GetAnonyString(devId).c_str());
    int32_t ret;
    {
        std::lock_guard<std::mutex> lock(dcallProxyMtx_);
        if (dcallProxy_ == nullptr) {
            TELEPHONY_LOGE("dcallProxy_ is nullptr");
            return;
        }
        ret = dcallProxy_->SwitchDevice(devId, DCALL_SWITCH_DEVICE_TYPE_SOURCE);
    }
    if (ret == TELEPHONY_SUCCESS) {
        dCallDeviceSwitchedOn_.store(false);
        ClearConnectedDCallDevice();
        TELEPHONY_LOGI("switch dcall device off succeed.");
    } else {
        TELEPHONY_LOGE("switch dcall device off failed, %{public}d", ret);
    }
}

bool DistributedCallManager::IsSelectVirtualModem()
{
    if (onlineDCallDevices_.size() <= 0) {
        TELEPHONY_LOGW("no dcall device");
        return false;
    }
    std::lock_guard<std::mutex> lock(dcallProxyMtx_);
    if (dcallProxy_ == nullptr) {
        TELEPHONY_LOGE("fail to create dcall proxy obj");
        return false;
    }
    return dcallProxy_->IsSelectVirtualModem();
}

void DistributedCallManager::ReportDistributedDeviceInfo(const AudioDevice& device)
{
    std::string curDevId = GetDevIdFromAudioDevice(device);
    TELEPHONY_LOGD("curDevId = %{public}s", GetAnonyString(curDevId).c_str());
    AudioSystemManager *audioSystemMananger = AudioSystemManager::GetInstance();
    if (audioSystemMananger == nullptr) {
        TELEPHONY_LOGW("audioSystemMananger nullptr");
        return;
    }
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> descs = audioSystemMananger
        ->GetDevices(DeviceFlag::DISTRIBUTED_OUTPUT_DEVICES_FLAG);
    size_t size = descs.size();
    if (descs.size() <= 0) {
        TELEPHONY_LOGW("no distributed device");
        return;
    }
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> remoteDevice = descs;
    for (auto device = descs.begin(); device != descs.end(); device++) {
        std::string devId = (*device)->networkId_;
        if (!devId.empty() && curDevId == devId) {
            TELEPHONY_LOGI("curDecId is the same as devId, devId = %{public}s",
                GetAnonyString(devId).c_str());
            remoteDevice.clear();
            remoteDevice.push_back(*device);
            break;
        }
    }
    sptr<AudioRendererFilter> audioRendererFilter = new(std::nothrow) AudioRendererFilter();
    if (audioRendererFilter == nullptr) {
        TELEPHONY_LOGE("audioRendererFilter nullptr");
        return;
    }
    audioRendererFilter->rendererInfo.contentType = ContentType::CONTENT_TYPE_SPEECH;
    audioRendererFilter->rendererInfo.streamUsage = StreamUsage::STREAM_USAGE_VOICE_MODEM_COMMUNICATION;
    audioSystemMananger->SelectOutputDevice(audioRendererFilter, remoteDevice);
    TELEPHONY_LOGW("ReportDistributedDeviceInfo");
}

void DistributedCallManager::ReportDistributedDeviceInfoForSwitchOff()
{
    TELEPHONY_LOGI("ReportDistributedDeviceInfoForSwitchOff start.");
    AudioSystemManager *audioSystemMananger = AudioSystemManager::GetInstance();
    if (audioSystemMananger == nullptr) {
        TELEPHONY_LOGW("audioSystemMananger nullptr");
        return;
    }
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> descs = audioSystemMananger
        ->GetDevices(DeviceFlag::OUTPUT_DEVICES_FLAG);
    size_t size = descs.size();
    if (descs.size() <= 0) {
        TELEPHONY_LOGW("no distributed device");
        return;
    }
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> remoteDevice = descs;
    for (auto device = descs.begin(); device != descs.end(); device++) {
        if ((*device)->deviceType_ == DeviceType::DEVICE_TYPE_EARPIECE) {
            TELEPHONY_LOGI("curDecType is speaker.");
            remoteDevice.clear();
            remoteDevice.push_back(*device);
            break;
        }
    }
    sptr<AudioRendererFilter> audioRendererFilter = new(std::nothrow) AudioRendererFilter();
    if (audioRendererFilter == nullptr) {
        TELEPHONY_LOGE("audioRendererFilter nullptr");
        return;
    }
    audioRendererFilter->rendererInfo.contentType = ContentType::CONTENT_TYPE_SPEECH;
    audioRendererFilter->rendererInfo.streamUsage = StreamUsage::STREAM_USAGE_VOICE_MODEM_COMMUNICATION;
    audioSystemMananger->SelectOutputDevice(audioRendererFilter, remoteDevice);
    TELEPHONY_LOGI("ReportDistributedDeviceInfoForSwitchOff end.");
}

bool DistributedCallManager::IsDCallDeviceSwitchedOn()
{
    return dCallDeviceSwitchedOn_.load();
}

int32_t DistributedCallManager::OnDCallDeviceOnline(const std::string &devId)
{
    TELEPHONY_LOGI("dcall device is online, devId: %{public}s", GetAnonyString(devId).c_str());
    return AddDCallDevice(devId);
}

int32_t DistributedCallManager::OnDCallDeviceOffline(const std::string &devId)
{
    TELEPHONY_LOGI("dcall device is offline, devId: %{public}s", GetAnonyString(devId).c_str());
    auto ret = RemoveDCallDevice(devId);
#ifdef ABILITY_BLUETOOTH_SUPPORT
    bool isAllDeviceOffline = true;
    {
        std::lock_guard<std::mutex> lock(onlineDeviceMtx_);
        isAllDeviceOffline = onlineDCallDevices_.empty();
    }
    if (isAllDeviceOffline) {
        TELEPHONY_LOGI("all dcall device offline");
        std::lock_guard<std::mutex> lock(mutex_);
        if (dcallHfpListener_ != nullptr) {
            Bluetooth::HandsFreeAudioGateway::GetProfile()->DeregisterObserver(dcallHfpListener_);
            dcallHfpListener_ = nullptr;
        }
    }
#endif
    return ret;
}

int32_t DistributedCallManager::DistributedCallDeviceListener::OnDCallDeviceOnline(const std::string &devId)
{
    TELEPHONY_LOGI("dcall device is online, devId: %{public}s", GetAnonyString(devId).c_str());
    return DelayedSingleton<DistributedCallManager>::GetInstance()->OnDCallDeviceOnline(devId);
}

int32_t DistributedCallManager::DistributedCallDeviceListener::OnDCallDeviceOffline(const std::string &devId)
{
    TELEPHONY_LOGI("dcall device is offline, devId: %{public}s", GetAnonyString(devId).c_str());
    return DelayedSingleton<DistributedCallManager>::GetInstance()->OnDCallDeviceOffline(devId);
}

void DistributedCallManager::OnDCallSystemAbilityAdded(const std::string &deviceId)
{
    TELEPHONY_LOGI("dcall source service is added, deviceId: %{public}s", GetAnonyString(deviceId).c_str());
    std::vector<std::string> dcallDevices;
    {
        std::lock_guard<std::mutex> lock(dcallProxyMtx_);
        dcallProxy_ = std::make_shared<DistributedCallProxy>();
        if (dcallProxy_ == nullptr) {
            TELEPHONY_LOGE("fail to create dcall proxy obj");
            return;
        }
        if (dcallProxy_->Init() != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGE("init dcall proxy failed");
            return;
        }
        dcallDeviceListener_ = std::make_shared<DistributedCallDeviceListener>();
        if (dcallDeviceListener_ == nullptr) {
            TELEPHONY_LOGE("dcallDeviceListener_ is nullptr");
            return;
        }
        if (dcallProxy_->RegisterDeviceCallback(CALLBACK_NAME, dcallDeviceListener_) != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGE("register dcall callback failed");
            return;
        }
        if (dcallProxy_->GetOnlineDeviceList(dcallDevices) != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGE("get dcall device list failed");
            return;
        }
    }
    if (dcallDevices.size() > 0) {
        NotifyOnlineDCallDevices(dcallDevices);
    }
    TELEPHONY_LOGI("OnDCallSystemAbilityAdded end.");
}

void DistributedCallManager::OnDCallSystemAbilityRemoved(const std::string &deviceId)
{
    TELEPHONY_LOGI("dcall source service is removed, deviceId: %{public}s", GetAnonyString(deviceId).c_str());
    {
        std::lock_guard<std::mutex> lock(dcallProxyMtx_);
        dcallDeviceListener_ = nullptr;
        dcallProxy_ = nullptr;
    }
    dCallDeviceSwitchedOn_.store(false);
    ClearDCallDevices();
    ClearConnectedDCallDevice();
    DelayedSingleton<AudioDeviceManager>::GetInstance()->ResetDistributedCallDevicesList();
    DelayedSingleton<AudioDeviceManager>::GetInstance()->InitAudioDevice();
    TELEPHONY_LOGI("OnDCallSystemAbilityRemoved end.");
}

void DCallSystemAbilityListener::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    TELEPHONY_LOGI("SA: %{public}d is added!", systemAbilityId);
    if (!CheckInputSysAbilityId(systemAbilityId)) {
        TELEPHONY_LOGE("added SA is invalid!");
        return;
    }
    if (systemAbilityId != DISTRIBUTED_CALL_SOURCE_SA_ID) {
        TELEPHONY_LOGE("added SA is not dcall source service, ignored.");
        return;
    }
    TELEPHONY_LOGI("notify dcall source service added event to distributed call manager");
    DelayedSingleton<DistributedCallManager>::GetInstance()->OnDCallSystemAbilityAdded(deviceId);
}

void DCallSystemAbilityListener::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    TELEPHONY_LOGI("SA: %{public}d is removed!", systemAbilityId);
    if (!CheckInputSysAbilityId(systemAbilityId)) {
        TELEPHONY_LOGE("removed SA is invalid!");
        return;
    }
    if (systemAbilityId != DISTRIBUTED_CALL_SOURCE_SA_ID) {
        TELEPHONY_LOGE("removed SA is not dcall source service, ignored.");
        return;
    }
    TELEPHONY_LOGI("notify dcall source service removed event to distributed call manager");
    DelayedSingleton<DistributedCallManager>::GetInstance()->OnDCallSystemAbilityRemoved(deviceId);
}

#ifdef ABILITY_BLUETOOTH_SUPPORT
void DCallHfpListener::OnHfpStackChanged(const Bluetooth::BluetoothRemoteDevice &device, int32_t action)
{
    TELEPHONY_LOGI("dcall hfp stack changed, action[%{public}d]", action);
    int32_t cod = DEFAULT_DCALL_HFP_FLAG_VALUE;
    int32_t majorClass = DEFAULT_DCALL_HFP_FLAG_VALUE;
    int32_t majorMinorClass = DEFAULT_DCALL_HFP_FLAG_VALUE;
    device.GetDeviceProductType(cod, majorClass, majorMinorClass);
    bool isBtHeadset = (majorClass == Bluetooth::BluetoothDevice::MAJOR_AUDIO_VIDEO &&
                        (majorMinorClass == Bluetooth::BluetoothDevice::AUDIO_VIDEO_HEADPHONES ||
                         majorMinorClass == Bluetooth::BluetoothDevice::AUDIO_VIDEO_WEARABLE_HEADSET));
    if (!isBtHeadset) {
        return;
    }
    if (action == DCALL_BT_HEADSET_UNWEAR_ACTION) {
        DelayedSingleton<AudioDeviceManager>::GetInstance()->CheckAndSwitchDistributedAudioDevice();
    }
}
#endif
 
} // namespace Telephony
} // namespace OHOS