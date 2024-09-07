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

#include "native_call_manager_utils.h"
#include "call_manager_base.h"
#include <securec.h>

namespace OHOS {
namespace Telephony {

CallAttributeInfo NativeCallManagerUtils::ReadCallAttributeInfo(MessageParcel &messageParcel)
{
    CallAttributeInfo info;
    strncpy_s(info.accountNumber, kMaxNumberLen + 1, messageParcel.ReadCString(), kMaxNumberLen + 1);
    strncpy_s(info.bundleName, kMaxNumberLen + 1, messageParcel.ReadCString(), kMaxNumberLen + 1);
    info.speakerphoneOn = messageParcel.ReadBool();
    info.accountId = messageParcel.ReadInt32();
    info.videoState = static_cast<VideoStateType>(messageParcel.ReadInt32());
    info.startTime = messageParcel.ReadInt64();
    info.isEcc = messageParcel.ReadBool();
    info.callType = static_cast<CallType>(messageParcel.ReadInt32());
    info.callId = messageParcel.ReadInt32();
    info.callState = static_cast<TelCallState>(messageParcel.ReadInt32());
    info.conferenceState = static_cast<TelConferenceState>(messageParcel.ReadInt32());
    info.callBeginTime = messageParcel.ReadInt64();
    info.callEndTime = messageParcel.ReadInt64();
    info.ringBeginTime = messageParcel.ReadInt64();
    info.ringEndTime = messageParcel.ReadInt64();
    info.callDirection = static_cast<CallDirection>(messageParcel.ReadInt32());
    info.answerType = static_cast<CallAnswerType>(messageParcel.ReadInt32());
    info.index = messageParcel.ReadInt32();
    info.crsType = messageParcel.ReadInt32();
    info.originalCallType = messageParcel.ReadInt32();
    strncpy_s(info.numberLocation, kMaxNumberLen + 1, messageParcel.ReadCString(), kMaxNumberLen + 1);
    info.numberMarkInfo.markType = static_cast<MarkType>(messageParcel.ReadInt32());
    strncpy_s(info.numberMarkInfo.markContent, kMaxNumberLen + 1, messageParcel.ReadCString(), kMaxNumberLen + 1);
    info.numberMarkInfo.markCount = messageParcel.ReadInt32();
    strncpy_s(info.numberMarkInfo.markSource, kMaxNumberLen + 1, messageParcel.ReadCString(), kMaxNumberLen + 1);
    info.numberMarkInfo.isCloud = messageParcel.ReadBool();
    if (info.callType == CallType::TYPE_VOIP) {
        info.voipCallInfo.voipCallId = messageParcel.ReadString();
        info.voipCallInfo.userName = messageParcel.ReadString();
        info.voipCallInfo.abilityName = messageParcel.ReadString();
        info.voipCallInfo.extensionId = messageParcel.ReadString();
        info.voipCallInfo.voipBundleName = messageParcel.ReadString();
        info.voipCallInfo.showBannerForIncomingCall = messageParcel.ReadBool();
        info.voipCallInfo.isConferenceCall = messageParcel.ReadBool();
        info.voipCallInfo.isVoiceAnswerSupported = messageParcel.ReadBool();
        info.voipCallInfo.hasMicPermission = messageParcel.ReadBool();
        info.voipCallInfo.uid = messageParcel.ReadInt32();
        std::vector<uint8_t> userProfile = {};
        messageParcel.ReadUInt8Vector(&userProfile);
        (info.voipCallInfo.userProfile).assign(userProfile.begin(), userProfile.end());
    }
    return info;
}

} // namespace Telephony
} // namespace OHOS