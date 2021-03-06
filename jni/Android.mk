# Copyright 2011 Tero Saarni
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := nativeegl
LOCAL_CFLAGS    := -Wall 
LOCAL_C_INCLUDES := $(LOCAL_PATH)/bgfx/include \
	$(LOCAL_PATH)/bgfx/3rdparty/khronos \
	$(LOCAL_PATH)/bx/include

LOCAL_SRC_FILES := jniapi.cpp renderer.cpp bgfx/src/amalgamated.cpp
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2 

include $(BUILD_SHARED_LIBRARY)
