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

#ifndef TELEPHONY_DISTRIBUTED_CALL_MANAGER_H
#define TELEPHONY_DISTRIBUTED_CALL_MANAGER_H

#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <atomic>

#include "singleton.h"
#include "idcall_device_callback.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"
#include "call_manager_inner_type.h"
#include "distributed_call_proxy.h"

namespace OHOS {
namespace Telephony {
class DistributedCallManager {
    DECLARE_DELAYED_SINGLETON(DistributedCallManager)
public:
    void Init();
    void AddDCallDevice(const std::string& devId);
    void RemoveDCallDevice(const std::string& devId);
    void ClearDCallDevice();
    void ClearConnectedDAudioDevice();
    std::string GetConnectedDCallAddr();
    AudioDeviceType GetConnectedDCallType();
    void DisconnectDCallDevice();
    bool IsDAudioDeviceConnected();
    bool SwitchDCallDevice(const AudioDevice& device);
    void SwitchDCallDeviceAsync(const AudioDevice& device);
    void SetCallState(bool isActive);

    void OnDeviceOnline(const std::string &devId);
    void OnDeviceOffline(const std::string &devId);
    void OnDCallSystemAbilityAdded(const std::string &deviceId);
    void OnDCallSystemAbilityRemoved(const std::string &deviceId);

private:
    class DistributedCallDeviceListener : public OHOS::DistributedHardware::IDCallDeviceCallback {
    public:
        DistributedCallDeviceListener() = default;
        ~DistributedCallDeviceListener() = default;

        int32_t OnDeviceOnline(const std::string &devId) override;
        int32_t OnDeviceOffline(const std::string &devId) override;
    };

    bool CreateDAudioDevice(const std::string& devId, AudioDevice& device);
    std::string GetDevIdFromAudioDevice(const AudioDevice& device);
    void NotifyOnlineDCallDevices(std::vector<std::string> devices);
    std::string GetConnectedDCallDeviceId();

    void GetConnectedAudioDevice(AudioDevice& device);
    void SetConnectedAudioDevice(const AudioDevice& device);
    void SwitchAudioDeviceToDistributedCar(std::unique_ptr<AudioDevice> carDevice);

private:
    std::atomic<bool> isCallActived_ = false;
    std::atomic<bool> isConnected_ = false;
    std::mutex connectedDevMtx_;
    std::mutex onlineDeviceMtx_;
    std::string connectedDevId_;
    AudioDevice connectedAudioDevice_;
    std::map<std::string, AudioDevice> onlineDCallDevices_;
    sptr<ISystemAbilityStatusChange> statusChangeListener_ = nullptr;
    std::shared_ptr<DistributedCallProxy> dcallProxy_ = nullptr;
    std::shared_ptr<DistributedCallDeviceListener> dcallDeviceListener_ = nullptr;
};

class DCallSystemAbilityListener : public SystemAbilityStatusChangeStub {
public:
    DCallSystemAbilityListener() = default;
    ~DCallSystemAbilityListener() = default;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
};
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_DISTRIBUTED_CALL_MANAGER_H