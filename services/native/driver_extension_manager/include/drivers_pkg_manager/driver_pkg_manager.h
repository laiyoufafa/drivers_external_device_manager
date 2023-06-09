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

#ifndef DRIVER_PKG_MANAGER_H
#define DRIVER_PKG_MANAGER_H

#include <iostream>
#include <stdint.h>

#include "bundle_monitor.h"
#include "drv_bundle_state_callback.h"
#include "ibus_extension.h"
#include "single_instance.h"
#include "ext_object.h"
namespace OHOS {
namespace ExternalDeviceManager {
using namespace std;
using namespace OHOS;
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::ExternalDeviceManager;

enum {
    ERR_DRV_PKG_MGR_ERROR = 1,
};

struct BundleInfoNames {
    string bundleName;
    string abilityName;
};

class DriverPkgManager {
    DECLARE_SINGLE_INSTANCE_BASE(DriverPkgManager);

public:
    void PrintTest();
    /**
     * @brief Called at first before Monitor DriverExtension Package.
     * @param bundleName Indicates the bundle name of the application.
     * @param launcherShortcutInfo List of LauncherShortcutInfo objects if obtained.
     * @return Returns true if the function is successfully called; returns false otherwise.
     */
    int32_t Init();
    shared_ptr<BundleInfoNames> QueryMatchDriver(shared_ptr<DeviceInfo> devInfo);
    int32_t RegisterOnBundleUpdate(PCALLBACKFUN pFun);
    int32_t UnRegisterOnBundleUpdate();
    ~DriverPkgManager();
private:
    shared_ptr<BundleMonitor> bundleMonitor_ = nullptr;
    sptr<DrvBundleStateCallback> bundleStateCallback_ = nullptr;
    BundleInfoNames bundleInfoName_;

    int32_t RegisterCallback(const sptr<IBundleStatusCallback> &callback);
    int32_t UnRegisterCallback();
    DriverPkgManager();
};
} // namespace ExternalDeviceManager
} // namespace OHOS
#endif // DRIVER_PKG_MANAGER_H