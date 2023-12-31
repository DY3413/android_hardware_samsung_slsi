/*
 **
 ** Copyright 2019, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 * LINT_KERNEL_FILE
 */

#include <android-base/logging.h>
#include <android/hardware/weaver/1.0/IWeaver.h>
#include <hidl/HidlTransportSupport.h>
#include <weaver_device.h>

int main() {
	::android::hardware::configureRpcThreadpool(1, true /* willJoinThreadpool */);

	WeaverDeviceImpl* impl = new WeaverDeviceImpl();
	android::sp <::android::hardware::weaver::V1_0::IWeaver> weaver =
		new ::android::hardware::weaver::V1_0::implementation::WeaverDevice(impl);

	auto status = weaver->registerAsService();
	if (status != android::OK) {
		LOG(FATAL) << "Could not register service for weaver 1.0 (" << status << ")";
	}

	android::hardware::joinRpcThreadpool();
	return -1;  // Should never get here.
}

