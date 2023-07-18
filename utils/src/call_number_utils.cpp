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

#include "call_number_utils.h"

#include <regex>

#include "phonenumbers/phonenumber.pb.h"
#include "telephony_log_wrapper.h"
#include "telephony_types.h"
#include "call_manager_errors.h"
#include "cellular_call_connection.h"

namespace OHOS {
namespace Telephony {
CallNumberUtils::CallNumberUtils() {}

CallNumberUtils::~CallNumberUtils() {}

int32_t CallNumberUtils::FormatPhoneNumber(
    const std::string &phoneNumber, const std::string &countryCode, std::string &formatNumber)
{
    if (phoneNumber.empty()) {
        TELEPHONY_LOGE("phoneNumber is nullptr!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (phoneNumber.front() == '#' || phoneNumber.front() == '*') {
        formatNumber = phoneNumber;
        return TELEPHONY_SUCCESS;
    }
    i18n::phonenumbers::PhoneNumberUtil *phoneUtils = i18n::phonenumbers::PhoneNumberUtil::GetInstance();
    if (phoneUtils == nullptr) {
        TELEPHONY_LOGE("phoneUtils is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::string tmpCode = countryCode;
    transform(tmpCode.begin(), tmpCode.end(), tmpCode.begin(), ::toupper);
    i18n::phonenumbers::PhoneNumber parseResult;
    phoneUtils->ParseAndKeepRawInput(phoneNumber, tmpCode, &parseResult);
    phoneUtils->FormatInOriginalFormat(parseResult, tmpCode, &formatNumber);
    if (formatNumber.empty() || formatNumber == "0") {
        TELEPHONY_LOGE("FormatPhoneNumber failed!");
        return CALL_ERR_FORMAT_PHONE_NUMBER_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallNumberUtils::FormatPhoneNumberToE164(
    const std::string phoneNumber, const std::string countryCode, std::string &formatNumber)
{
    return FormatNumberBase(phoneNumber, countryCode, i18n::phonenumbers::PhoneNumberUtil::E164, formatNumber);
}

int32_t CallNumberUtils::FormatNumberBase(const std::string phoneNumber, std::string countryCode,
    const i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat formatInfo, std::string &formatNumber)
{
    if (phoneNumber.empty()) {
        TELEPHONY_LOGE("phoneNumber is nullptr!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    i18n::phonenumbers::PhoneNumberUtil *phoneUtils = i18n::phonenumbers::PhoneNumberUtil::GetInstance();
    if (phoneUtils == nullptr) {
        TELEPHONY_LOGE("phoneUtils is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::toupper);
    i18n::phonenumbers::PhoneNumber parseResult;
    phoneUtils->Parse(phoneNumber, countryCode, &parseResult);
    if (phoneUtils->IsValidNumber(parseResult)) {
        phoneUtils->Format(parseResult, formatInfo, &formatNumber);
    }
    if (formatNumber.empty() || formatNumber == "0") {
        TELEPHONY_LOGE("FormatPhoneNumber failed!");
        return CALL_ERR_FORMAT_PHONE_NUMBER_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallNumberUtils::CheckNumberIsEmergency(const std::string &phoneNumber, const int32_t slotId, bool &enabled)
{
    return DelayedSingleton<CellularCallConnection>::GetInstance()->IsEmergencyPhoneNumber(
        phoneNumber, slotId, enabled);
}

bool CallNumberUtils::IsValidSlotId(int32_t slotId) const
{
    if (SIM_SLOT_COUNT == HAS_A_SLOT) {
        return slotId == SIM_SLOT_0;
    }
    if (SIM_SLOT_COUNT == HAS_TWO_SLOT) {
        if (slotId == SIM_SLOT_0 || slotId == SIM_SLOT_1) {
            return true;
        }
    }
    return false;
}

bool CallNumberUtils::IsMMICode(const std::string &number)
{
    if (number.empty()) {
        TELEPHONY_LOGE("number is empty.");
        return false;
    }
    if (RegexMatchMmi(number)) {
        return true;
    }

    if (number.back() == '#') {
        TELEPHONY_LOGI("number is end of #");
        return true;
    }

    return false;
}

bool CallNumberUtils::RegexMatchMmi(const std::string &number)
{
    std::string symbols =
        "((\\*|#|\\*#|\\*\\*|##)(\\d{2,3})(\\*([^*#]*)(\\*([^*#]*)(\\*([^*#]*)(\\*([^*#]*))?)?)?)?#)(.*)";
    std::regex pattern(symbols);
    std::smatch results;
    if (regex_match(number, results, pattern)) {
        TELEPHONY_LOGI("regex_match ture");
        return true;
    }
    return false;
}

std::string CallNumberUtils::RemoveSeparatorsPhoneNumber(const std::string &phoneString)
{
    std::string newString;
    if (phoneString.empty()) {
        TELEPHONY_LOGE("RemoveSeparatorsPhoneNumber return, phoneStr is empty.");
        return newString;
    }
    for (char c : phoneString) {
        if ((c >= '0' && c <= '9') || c == '*' || c == '#' || c == '+' || c == 'N' || c == ',' || c == ';') {
            newString += c;
        }
    }

    return newString;
}
} // namespace Telephony
} // namespace OHOS
