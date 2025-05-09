/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "cJSON.h"
#include "cs_call.h"
#include "common_type.h"
#include "telephony_errors.h"
#include "call_manager_base.h"
#include "distributed_data_controller.h"
#include "distributed_device_observer.h"
#include "distributed_communication_manager.h"
#include "call_manager_disconnected_details.h"
#include "distributed_data_sink_controller.h"
#include "distributed_sink_switch_controller.h"

namespace OHOS {
namespace Telephony {
using namespace testing::ext;

class DistributedCommunicationManagerTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    virtual void SetUp() {}
    virtual void TearDown() {}
};

/**
 * @tc.number   Telephony_DcManagerTest_001
 * @tc.name     test normal branch
 * @tc.desc     normal branch test
 */
HWTEST_F(DistributedCommunicationManagerTest, Telephony_DcManagerTest_001, Function | MediumTest | Level1)
{
    int32_t direction = 1;
    DialParaInfo mDialParaInfo;
    DisconnectedDetails details;
    sptr<CallBase> csCall = nullptr;
    std::string devId = "UnitTestDeviceId";
    auto dcManager = DelayedSingleton<DistributedCommunicationManager>::GetInstance();
    std::shared_ptr<IDistributedDeviceCallback> deviceListener = std::make_shared<DistributedDeviceCallback>();

    ASSERT_NO_THROW(dcManager->NewCallCreated(csCall));

    auto deviceObserver = dcManager->GetDistributedDeviceObserver();
    ASSERT_TRUE(deviceObserver == nullptr);

    ASSERT_NO_THROW(dcManager->Init());
    csCall = new CSCall(mDialParaInfo);

    int32_t ret = dcManager->RegDevCallbackWrapper(deviceListener);
    ASSERT_TRUE(ret == TELEPHONY_ERROR);

    ret = dcManager->UnRegDevCallbackWrapper();
    ASSERT_TRUE(ret == TELEPHONY_ERROR);

    ret = dcManager->SwitchDevWrapper(devId, direction);
    ASSERT_TRUE(ret == TELEPHONY_ERROR);

    ASSERT_NO_THROW(dcManager->InitExtWrapper());
    ASSERT_NO_THROW(dcManager->InitExtWrapper()); // extWrapperHandler_ != nullptr case

    ret = dcManager->RegDevCallbackWrapper(deviceListener);
    ASSERT_TRUE(ret != TELEPHONY_ERROR);

    ret = dcManager->UnRegDevCallbackWrapper();
    ASSERT_TRUE(ret != TELEPHONY_ERROR);

    ret = dcManager->SwitchDevWrapper(devId, direction);
    ASSERT_TRUE(ret != TELEPHONY_ERROR);

    ASSERT_FALSE(dcManager->IsDistributedDev(devId));

    deviceObserver = dcManager->GetDistributedDeviceObserver();
    ASSERT_TRUE(deviceObserver != nullptr);
    ASSERT_NO_THROW(dcManager->SetMuted(true));
    ASSERT_NO_THROW(dcManager->MuteRinger());
    ASSERT_NO_THROW(dcManager->ProcessCallInfo(csCall, DistributedDataType::LOCATION));
    ASSERT_NO_THROW(dcManager->NewCallCreated(csCall));
    ASSERT_NO_THROW(dcManager->CallDestroyed(details));
    ASSERT_NO_THROW(dcManager->DeInitExtWrapper());
    ASSERT_NO_THROW(dcManager->dataController_ = std::make_shared<DistributedDataSinkController>());
    ASSERT_NO_THROW(dcManager->SetMuted(true));
    ASSERT_NO_THROW(dcManager->MuteRinger());
    ASSERT_NO_THROW(dcManager->ProcessCallInfo(csCall, DistributedDataType::LOCATION));
    csCall->SetCallType(CallType::TYPE_CS);
    dcManager->role_ = DistributedRole::SINK;
    ASSERT_NO_THROW(dcManager->NewCallCreated(csCall));
    dcManager->peerDevices_.push_back("device");
    ASSERT_NO_THROW(dcManager->NewCallCreated(csCall));
    ASSERT_NO_THROW(dcManager->CallDestroyed(details));
    ASSERT_NO_THROW(dcManager->peerDevices_.clear());
}

/**
 * @tc.number   Telephony_DcManager_DeviceOnline_001
 * @tc.name     test dc device online
 * @tc.desc     Function test
 */
HWTEST_F(DistributedCommunicationManagerTest, Telephony_DcManager_DeviceOnline_001, Function | MediumTest | Level1)
{
    int32_t devRole = 0; // sink
    std::string devId = "UnitTestDeviceId";
    std::string devName = "UnitTestDeviceName";
    AudioDeviceType deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PHONE;

    auto dcManager = DelayedSingleton<DistributedCommunicationManager>::GetInstance();
    dcManager->devObserver_ = nullptr;
    ASSERT_NO_THROW(dcManager->OnDeviceOnline(devId, devName, deviceType, devRole));
    ASSERT_FALSE(dcManager->IsConnected());

    ASSERT_NO_THROW(dcManager->Init());

    ASSERT_NO_THROW(dcManager->OnDeviceOnline(devId, devName, deviceType, devRole));
    ASSERT_TRUE(dcManager->IsConnected());
    ASSERT_TRUE(dcManager->IsSinkRole());

    devRole = 1; // source
    deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PAD;
    dcManager->dataController_ = nullptr;
    ASSERT_NO_THROW(dcManager->OnDeviceOnline(devId, devName, deviceType, devRole));
    ASSERT_FALSE(dcManager->IsSinkRole());
}

/**
 * @tc.number   Telephony_DcManager_DeviceOffline_001
 * @tc.name     test dc device offline
 * @tc.desc     Function test
 */
HWTEST_F(DistributedCommunicationManagerTest, Telephony_DcManager_DeviceOffline_001, Function | MediumTest | Level1)
{
    int32_t devRole = 0;
    std::string devId = "UnitTestDeviceId";
    std::string devName = "UnitTestDeviceName";
    AudioDeviceType deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PHONE;

    auto dcManager = DelayedSingleton<DistributedCommunicationManager>::GetInstance();
    ASSERT_NO_THROW(dcManager->OnDeviceOffline(devId, devName, deviceType, devRole));
    ASSERT_NO_THROW(dcManager->IsConnected());

    ASSERT_NO_THROW(dcManager->Init());
    ASSERT_NO_THROW(dcManager->OnDeviceOnline(devId, devName, deviceType, devRole));
    ASSERT_TRUE(dcManager->IsConnected());

    ASSERT_NO_THROW(dcManager->OnDeviceOffline(devId, devName, deviceType, devRole));
    ASSERT_FALSE(dcManager->IsConnected());

    dcManager->devObserver_ = nullptr;
    ASSERT_NO_THROW(dcManager->OnDeviceOffline(devId, devName, deviceType, devRole));

    dcManager->devObserver_ = std::make_shared<DistributedDeviceObserver>();
    dcManager->peerDevices_.push_back("hello");
    ASSERT_NO_THROW(dcManager->OnDeviceOffline(devId, devName, deviceType, devRole));
    dcManager->peerDevices_.clear();
}

/**
 * @tc.number   Telephony_DcManager_SwitchDevice_001
 * @tc.name     test switch device
 * @tc.desc     Function test
 */
HWTEST_F(DistributedCommunicationManagerTest, Telephony_DcManager_SwitchDevice_001, Function | MediumTest | Level1)
{
    int32_t devRole = 1;
    std::string devId = "UnitTestDeviceId";
    std::string devName = "UnitTestDeviceName";
    AudioDeviceType deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PAD;
    auto dcManager = DelayedSingleton<DistributedCommunicationManager>::GetInstance();
    ASSERT_NO_THROW(dcManager->SwitchToSourceDevice());
    ASSERT_NO_THROW(dcManager->Init());
    ASSERT_NO_THROW(dcManager->OnDeviceOnline(devId, devName, deviceType, devRole));
    ASSERT_TRUE(dcManager->SwitchToSourceDevice());
    dcManager->devSwitchController_->isAudioOnSink_ = true;
    ASSERT_NO_THROW(dcManager->SwitchToSourceDevice());
    dcManager->devSwitchController_->isAudioOnSink_ = false;
}

/**
 * @tc.number   Telephony_DcManager_SwitchToSinkDevice
 * @tc.name     test switch to sink device
 * @tc.desc     Function test
 */
HWTEST_F(DistributedCommunicationManagerTest, Telephony_DcManager_SwitchToSinkDevice, Function | MediumTest | Level1)
{
    AudioDevice device;
    std::string deviceId = "{ \"devId\": \"101\" }";
    std::string deviceName = "deviceName";
    device.deviceType = AudioDeviceType::DEVICE_DISTRIBUTED_PAD;
    auto dcManager = DelayedSingleton<DistributedCommunicationManager>::GetInstance();
    ASSERT_FALSE(dcManager->SwitchToSinkDevice(device)); // deviceId is empty
    ASSERT_EQ(memcpy_s(device.address, kMaxAddressLen + 1, deviceId.c_str(), deviceId.size()), EOK);
    ASSERT_EQ(memcpy_s(device.deviceName, kMaxDeviceNameLen + 1, deviceName.c_str(), deviceName.size()), EOK);
    ASSERT_FALSE(dcManager->SwitchToSinkDevice(device)); // not dc device
    dcManager->peerDevices_.push_back("101");
    ASSERT_FALSE(dcManager->SwitchToSinkDevice(device));
    dcManager->devSwitchController_ = nullptr;
    ASSERT_FALSE(dcManager->SwitchToSinkDevice(device));
    dcManager->devSwitchController_ = std::make_shared<DistributedSinkSwitchController>();
    dcManager->devSwitchController_->isAudioOnSink_ = true;
    ASSERT_TRUE(dcManager->SwitchToSinkDevice(device));
    dcManager->devSwitchController_->isAudioOnSink_ = false;
    dcManager->peerDevices_.clear();
    deviceId = "not Json string";
    ASSERT_EQ(memcpy_s(device.address, kMaxAddressLen + 1, deviceId.c_str(), deviceId.size()), EOK);
    ASSERT_EQ(dcManager->ParseDevIdFromAudioDevice(device), "");
}
} // namespace Telephony
} // namespace OHOS
