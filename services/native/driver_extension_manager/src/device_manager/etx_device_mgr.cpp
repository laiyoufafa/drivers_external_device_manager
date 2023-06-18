/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "etx_device_mgr.h"
#include "cinttypes"
#include "driver_extension_controller.h"
#include "driver_pkg_manager.h"
#include "edm_errors.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace ExternalDeviceManager {
IMPLEMENT_SINGLE_INSTANCE(ExtDeviceManager);

void ExtDeviceManager::PrintMatchDriverMap()
{
    if (!bundleMatchMap_.empty()) {
        EDM_LOGD(MODULE_DEV_MGR, " bundleInfo map size[%{public}zu]", bundleMatchMap_.size());
        for (auto iter : bundleMatchMap_) {
            for (auto iterId = iter.second.begin(); iterId != iter.second.end(); ++iterId) {
                uint64_t deviceId = *iterId;
                EDM_LOGD(MODULE_DEV_MGR, "print match map info[%{public}s], deviceId %{public}016" PRIX64 "",
                    iter.first.c_str(), deviceId);
            }
        }
        EDM_LOGD(MODULE_DEV_MGR, "ExtDeviceManager print driver match map success");
    }
}

int32_t ExtDeviceManager::Init()
{
    EDM_LOGD(MODULE_DEV_MGR, "ExtDeviceManager Init success");
    return EDM_OK;
}

string ExtDeviceManager::GetBundleName(string &bundleInfo) const
{
    string::size_type pos = bundleInfo.find(stiching_);
    if (pos == string::npos) {
        EDM_LOGI(MODULE_DEV_MGR, "bundleInfo not find stiching name");
        return "";
    }

    return bundleInfo.substr(0, pos);
}

string ExtDeviceManager::GetAbilityName(string &bundleInfo) const
{
    string::size_type pos = bundleInfo.find(stiching_);
    if (pos == string::npos) {
        EDM_LOGI(MODULE_DEV_MGR, "bundleInfo not find stiching name");
        return "";
    }

    return bundleInfo.substr(pos + stiching_.length());
}

int32_t ExtDeviceManager::AddDevIdOfBundleInfoMap(uint64_t deviceId, string &bundleInfo)
{
    if (bundleInfo.empty()) {
        EDM_LOGE(MODULE_DEV_MGR, "bundleInfo is null");
        return EDM_NOK;
    }

    // update bundle info
    lock_guard<mutex> lock(bundleMatchMapMutex_);
    auto pos = bundleMatchMap_.find(bundleInfo);
    if (pos == bundleMatchMap_.end()) {
        unordered_set<uint64_t> tmpSet;
        tmpSet.emplace(deviceId);
        bundleMatchMap_.emplace(bundleInfo, tmpSet);
        EDM_LOGD(MODULE_DEV_MGR, "bundleMap emplace New driver, add deviceId %{public}016" PRIX64 "", deviceId);
    } else {
        auto pairRet = pos->second.emplace(deviceId);
        // Check whether the deviceId matches the driver
        if (!pairRet.second || pos->second.size() > 1) {
            EDM_LOGD(MODULE_DEV_MGR, "bundleMap had existed driver, add deviceId %{public}016" PRIX64 "", deviceId);
            PrintMatchDriverMap();
            return EDM_OK;
        }
    }

    PrintMatchDriverMap();
    return EDM_OK;
}

int32_t ExtDeviceManager::RemoveDevIdOfBundleInfoMap(uint64_t deviceId, string &bundleInfo)
{
    if (bundleInfo.empty()) {
        EDM_LOGE(MODULE_DEV_MGR, "bundleInfo is null");
        return EDM_NOK;
    }

    // update bundle info
    lock_guard<mutex> lock(bundleMatchMapMutex_);
    auto pos = bundleMatchMap_.find(bundleInfo);
    if (pos == bundleMatchMap_.end()) {
        EDM_LOGI(MODULE_DEV_MGR, "not find bundleInfo from map");
        return EDM_OK;
    }

    // If the number of devices is greater than one, only the device erase
    if (pos->second.size() > 1) {
        pos->second.erase(deviceId);
        EDM_LOGD(MODULE_DEV_MGR, "bundleMap existed driver, remove deviceId %{public}016" PRIX64 "", deviceId);
        PrintMatchDriverMap();
        return EDM_OK;
    }

    EDM_LOGD(MODULE_DEV_MGR, "bundleMap remove bundleInfo[%{public}s]", bundleInfo.c_str());
    bundleMatchMap_.erase(pos);

    PrintMatchDriverMap();
    return EDM_OK;
}

int32_t ExtDeviceManager::RemoveAllDevIdOfBundleInfoMap(string &bundleInfo)
{
    // update bundle info
    lock_guard<mutex> lock(bundleMatchMapMutex_);
    auto pos = bundleMatchMap_.find(bundleInfo);
    if (pos == bundleMatchMap_.end()) {
        return EDM_OK;
    }

    bundleMatchMap_.erase(pos);

    return EDM_OK;
}

int32_t ExtDeviceManager::AddBundleInfo(enum BusType busType, const string &bundleName, const string &abilityName)
{
    if (busType <= BUS_TYPE_INVALID || busType > BUS_TYPE_TEST) {
        EDM_LOGE(MODULE_DEV_MGR, "busType para invalid");
        return EDM_ERR_INVALID_PARAM;
    }

    if (bundleName.empty() || abilityName.empty()) {
        EDM_LOGE(MODULE_DEV_MGR, "BundleInfo para invalid");
        return EDM_ERR_INVALID_PARAM;
    }

    // iterate over device, find bundleInfo and ability status
    lock_guard<mutex> lock(deviceMapMutex_);

    // find device
    if (deviceMap_.count(busType) == 0) {
        EDM_LOGD(MODULE_DEV_MGR, "deviceMap_ not bustype[%{public}d], wait to plug device", busType);
        return EDM_OK;
    }

    string bundleInfo = bundleName + stiching_ + abilityName;
    unordered_map<uint64_t, shared_ptr<Device>> &map = deviceMap_[busType];

    for (auto iter : map) {
        shared_ptr<Device> device = iter.second;

        // iterate over device by bustype
        shared_ptr<BundleInfoNames> bundleInfoNames = make_shared<BundleInfoNames>();
        bundleInfoNames->bundleName = "bundle_name_test";
        bundleInfoNames->abilityName = "ability_name_test";

        if (bundleName.compare(bundleInfoNames->bundleName) == 0 &&
            abilityName.compare(bundleInfoNames->abilityName) == 0) {
            device->AddBundleInfo(bundleInfo);
            int32_t ret = AddDevIdOfBundleInfoMap(device->GetDeviceInfo()->GetDeviceId(), bundleInfo);
            if (ret != EDM_OK) {
                EDM_LOGE(MODULE_DEV_MGR,
                "deviceId[%{public}016" PRIX64 "] start driver extension ability[%{public}s] fail[%{public}d]",
                device->GetDeviceInfo()->GetDeviceId(), GetAbilityName(bundleInfo).c_str(), ret);
            }
        }
    }

    return EDM_OK;
}

int32_t ExtDeviceManager::RemoveBundleInfo(enum BusType busType, const string &bundleName, const string &abilityName)
{
    if (busType <= BUS_TYPE_INVALID || busType >= BUS_TYPE_TEST) {
        EDM_LOGE(MODULE_DEV_MGR, "busType para invalid");
        return EDM_ERR_INVALID_PARAM;
    }

    if (bundleName.empty() || abilityName.empty()) {
        EDM_LOGE(MODULE_DEV_MGR, "BundleInfo para invalid");
        return EDM_ERR_INVALID_PARAM;
    }

    lock_guard<mutex> lock(deviceMapMutex_);
    if (deviceMap_.count(busType) == 0) {
        EDM_LOGD(MODULE_DEV_MGR, "deviceMap_ not bustype[%{public}d], wait to plug device", busType);
        return EDM_OK;
    }

    // iterate over device, remove bundleInfo
    string bundleInfo = bundleName + stiching_ + abilityName;
    unordered_map<uint64_t, shared_ptr<Device>> &deviceMap = deviceMap_[busType];

    for (auto iter : deviceMap) {
        shared_ptr<Device> device = iter.second;

        // iterate over device by bustype
        if (bundleInfo.compare(device->GetBundleInfo()) == 0) {
            device->RemoveBundleInfo(); // update device
            int32_t ret = RemoveAllDevIdOfBundleInfoMap(bundleInfo);
            if (ret != EDM_OK) {
                EDM_LOGE(MODULE_DEV_MGR,
                "deviceId[%{public}016" PRIX64 "] stop driver extension ability[%{public}s] fail[%{public}d]",
                device->GetDeviceInfo()->GetDeviceId(), GetAbilityName(bundleInfo).c_str(), ret);
            }
        }
    }
    return EDM_OK;
}

int32_t ExtDeviceManager::UpdateBundleInfo(enum BusType busType, const string &bundleName, const string &abilityName)
{
    // stop ability of device and reset bundleInfo of device
    int32_t ret = RemoveBundleInfo(busType, bundleName, abilityName);
    if (ret != EDM_OK) {
        EDM_LOGE(MODULE_DEV_MGR, "remove bundle info fail");
        return EDM_NOK;
    }

    // iterate over device, add bundleInfo and start ability
    ret = AddBundleInfo(busType, bundleName, abilityName);
    if (ret != EDM_OK) {
        EDM_LOGE(MODULE_DEV_MGR, "add bundle info fail");
        return EDM_NOK;
    }

    return EDM_OK;
}

int32_t ExtDeviceManager::UpdateBundleStatusCallback(int32_t bundleStatus, int32_t busType,
    const string &bundleName, const string &abilityName)
{
    int32_t ret = EDM_NOK;
    if (bundleStatus < BUNDLE_ADDED || bundleStatus > BUNDLE_REMOVED) {
        EDM_LOGE(MODULE_DEV_MGR, "bundleStatus para invalid");
        return EDM_ERR_INVALID_PARAM;
    }

    // add bundle
    if (bundleStatus == BUNDLE_ADDED) {
        ret = ExtDeviceManager::GetInstance().AddBundleInfo((enum BusType)busType, bundleName, abilityName);
        if (ret != EDM_OK) {
            EDM_LOGE(MODULE_DEV_MGR, "callback add bundle info fail");
        }
        return ret;
    }

    // update bundle
    if (bundleStatus == BUNDLE_UPDATED) {
        ret = ExtDeviceManager::GetInstance().UpdateBundleInfo((enum BusType)busType, bundleName, abilityName);
        if (ret != EDM_OK) {
            EDM_LOGE(MODULE_DEV_MGR, "callback update bundle info fail");
        }
        return ret;
    }

    // remove bundle
    ret = ExtDeviceManager::GetInstance().RemoveBundleInfo((enum BusType)busType, bundleName, abilityName);
    if (ret != EDM_OK) {
        EDM_LOGE(MODULE_DEV_MGR, "callback update bundle info fail");
    }

    return ret;
}

int32_t ExtDeviceManager::RegisterDevice(shared_ptr<DeviceInfo> devInfo)
{
    BusType type = devInfo->GetBusType();
    uint64_t deviceId = devInfo->GetDeviceId();

    lock_guard<mutex> lock(deviceMapMutex_);
    if (deviceMap_.find(type) != deviceMap_.end()) {
        unordered_map<uint64_t, shared_ptr<Device>> &map = deviceMap_[type];
        if (map.find(deviceId) != map.end()) {
            EDM_LOGI(MODULE_DEV_MGR, "device has been registered, deviceId is %{public}016" PRIx64 "", deviceId);
            return EDM_OK;
        }
    }
    EDM_LOGD(MODULE_DEV_MGR, "device begin register deviceId is %{public}016" PRIx64 "", deviceId);

    // driver match
    shared_ptr<BundleInfoNames> bundleInfoNames = make_shared<BundleInfoNames>();
    bundleInfoNames->bundleName = "bundle_name_test";
    bundleInfoNames->abilityName = "ability_name_test";

    // add device
    shared_ptr<Device> device = make_shared<Device>(devInfo);
    string bundleInfo;

    // add bundle info
    if (bundleInfoNames != nullptr) {
        bundleInfo = bundleInfoNames->bundleName + stiching_ + bundleInfoNames->abilityName;
        device->AddBundleInfo(bundleInfo);
    }

    deviceMap_[type].emplace(deviceId, device);
    EDM_LOGD(MODULE_DEV_MGR, "successfully registered device, deviceId = %{public}016" PRIx64 "", deviceId);

    // not match driver, waitting install driver
    if (bundleInfo.empty()) {
        EDM_LOGD(MODULE_DEV_MGR, "deviceId %{public}016" PRIX64 "not match driver, waitting for install ext driver",
            deviceId);
        return EDM_OK;
    }

    int32_t ret = AddDevIdOfBundleInfoMap(deviceId, bundleInfo);
    if (ret != EDM_OK) {
        EDM_LOGE(MODULE_DEV_MGR, "deviceId[%{public}016" PRIX64 "] update bundle info map failed[%{public}d]",
            deviceId, ret);
        return EDM_NOK;
    }
    EDM_LOGD(MODULE_DEV_MGR, "successfully match driver[%{public}s], deviceId is %{public}016" PRIx64 "",
        bundleInfo.c_str(), deviceId);

    return ret;
}

int32_t ExtDeviceManager::UnRegisterDevice(const shared_ptr<DeviceInfo> devInfo)
{
    BusType type = devInfo->GetBusType();
    uint64_t deviceId = devInfo->GetDeviceId();
    string bundleInfo;

    lock_guard<mutex> lock(deviceMapMutex_);
    if (deviceMap_.find(type) != deviceMap_.end()) {
        unordered_map<uint64_t, shared_ptr<Device>> &map = deviceMap_[type];
        if (map.find(deviceId) != map.end()) {
            bundleInfo = map[deviceId]->GetBundleInfo();
            map.erase(deviceId);
            EDM_LOGI(MODULE_DEV_MGR, "successfully unregistered device, deviceId is %{public}016" PRIx64 "", deviceId);
        }
    }

    if (bundleInfo.empty()) {
        EDM_LOGD(MODULE_DEV_MGR, "deviceId %{public}016" PRIX64 "not find device", deviceId);
        return EDM_OK;
    }

    int32_t ret = RemoveDevIdOfBundleInfoMap(deviceId, bundleInfo);
    if (ret != EDM_OK) {
        EDM_LOGE(MODULE_DEV_MGR, "deviceId[%{public}016" PRIX64 "] remove bundleInfo map failed[%{public}d]",
            deviceId, ret);
        return ret;
    }

    EDM_LOGI(MODULE_DEV_MGR, "successfully remove bundleInfo, deviceId %{public}016" PRIx64 ", bundleInfo[%{public}s]",
        deviceId, bundleInfo.c_str());

    return EDM_OK;
}

vector<shared_ptr<DeviceInfo>> ExtDeviceManager::QueryDevice(const BusType busType)
{
    vector<shared_ptr<DeviceInfo>> devInfoVec;

    lock_guard<mutex> lock(deviceMapMutex_);
    if (deviceMap_.find(busType) == deviceMap_.end()) {
        EDM_LOGE(MODULE_DEV_MGR, "no device is found and busType %{public}d is invalid", busType);
        return devInfoVec;
    }

    unordered_map<uint64_t, shared_ptr<Device>> map = deviceMap_[busType];
    for (auto device : map) {
        devInfoVec.emplace_back(device.second->GetDeviceInfo());
    }
    EDM_LOGD(MODULE_DEV_MGR, "find %{public}zu device of busType %{public}d", devInfoVec.size(), busType);

    return devInfoVec;
}
} // namespace ExternalDeviceManager
} // namespace OHOS