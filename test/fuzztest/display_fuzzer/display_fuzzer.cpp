/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "display_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include "addcalltoken_fuzzer.h"
#include "fold_status_manager.h"

using namespace OHOS::Telephony;
namespace OHOS {

void FoldStatusManagerFunc(const uint8_t *data, size_t size)
{
    std::shared_ptr<FoldStatusManager> foldStatusManager = DelayedSingleton<FoldStatusManager>::GetInstance();
    foldStatusManager->UnregisterFoldableListener();
    foldStatusManager->RegisterFoldableListener();
    foldStatusManager->RegisterFoldableListener();
    foldStatusManager->UnregisterFoldableListener();
    FoldStatusManager::IsSmallFoldDevice();
}

void DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
    FoldStatusManagerFunc(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::AddCallTokenFuzzer token;
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
