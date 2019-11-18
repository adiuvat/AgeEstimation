/**
 * ============================================================================
 *
 * Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#include "Mind_camera.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cstring>
#include <chrono>
#include<ctime>

#include "hiaiengine/log.h"
extern "C" {
#include "driver/peripheral_api.h"
}

using hiai::Engine;
using namespace hiai;
using namespace std;

namespace {
// initial value of frameId
const uint32_t kInitFrameId = 0;
}

// register custom data type
HIAI_REGISTER_DATA_TYPE("AgeEstimationInfo", AgeEstimationInfo);

Mind_camera::Mind_camera() {
  config_ = nullptr;
  frame_id_ = kInitFrameId;
  exit_flag_ = CAMERADATASETS_INIT;
  InitConfigParams();
}

Mind_camera::~Mind_camera() {
}

string Mind_camera::CameraDatasetsConfig::ToString() const {
  stringstream log_info_stream("");
  log_info_stream << "fps:" << this->fps << ", camera:" << this->channel_id
                  << ", image_format:" << this->image_format
                  << ", resolution_width:" << this->resolution_width
                  << ", resolution_height:" << this->resolution_height;

  return log_info_stream.str();
}

HIAI_StatusT Mind_camera::Init(
    const hiai::AIConfig& ai_config,
    const vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG("[CameraDatasets] start init!");
  if (config_ == nullptr) {
    config_ = make_shared<CameraDatasetsConfig>();
  }

  for (int index = 0; index < ai_config.items_size(); ++index) {
    const ::hiai::AIConfigItem& item = ai_config.items(index);
    string name = item.name();
    string value = item.value();

    if (name == "fps") {
      config_->fps = atoi(value.data());
    } else if (name == "image_format") {
      config_->image_format = CommonParseParam(value);
    } else if (name == "data_source") {
      config_->channel_id = CommonParseParam(value);
    } else if (name == "image_size") {
      ParseImageSize(value, config_->resolution_width,
                     config_->resolution_height);
    } else {
      HIAI_ENGINE_LOG("unused config name: %s", name.c_str());
    }
  }

  HIAI_StatusT ret = HIAI_OK;
  bool failed_flag = (config_->image_format == PARSEPARAM_FAIL
      || config_->channel_id == PARSEPARAM_FAIL
      || config_->resolution_width == 0 || config_->resolution_height == 0);

  if (failed_flag) {
    string msg = config_->ToString();
    msg.append(" config data failed");
    HIAI_ENGINE_LOG(msg.data());
    ret = HIAI_ERROR;
  }

  HIAI_ENGINE_LOG("[CameraDatasets] end init!");
  return ret;
}

void Mind_camera::InitConfigParams() {
  params_.insert(pair<string, string>("Channel-1", IntToString(CAMERAL_1)));
  params_.insert(pair<string, string>("Channel-2", IntToString(CAMERAL_2)));
  params_.insert(
      pair<string, string>("YUV420SP", IntToString(CAMERA_IMAGE_YUV420_SP)));
}

string Mind_camera::IntToString(int value) {
  char msg[MAX_VALUESTRING_LENGTH] = { 0 };
  // MAX_VALUESTRING_LENGTH ensure no error occurred
  sprintf_s(msg, MAX_VALUESTRING_LENGTH, "%d", value);
  string ret = msg;

  return ret;
}

int Mind_camera::CommonParseParam(const string& val) const {
  map<string, string>::const_iterator iter = params_.find(val);
  if (iter != params_.end()) {
    return atoi((iter->second).c_str());
  }

  return PARSEPARAM_FAIL;
}

void Mind_camera::SplitString(const string& source, vector<string>& tmp,
                              const string& obj) {
  string::size_type pos1 = 0;
  string::size_type pos2 = source.find(obj);

  while (string::npos != pos2) {
    tmp.push_back(source.substr(pos1, pos2 - pos1));
    pos1 = pos2 + obj.size();
    pos2 = source.find(obj, pos1);
  }

  if (pos1 != source.length()) {
    tmp.push_back(source.substr(pos1));
  }
}

void Mind_camera::ParseImageSize(const string& val, int& width,
                                 int& height) const {
  vector<string> tmp;
  SplitString(val, tmp, "x");

  // val is not a format of resolution ratio(*x*),correct should have 2 array
  // in this wrong case,set width and height zero
  if (tmp.size() != 2) {
    width = 0;
    height = 0;
  } else {
    width = atoi(tmp[0].c_str());
    height = atoi(tmp[1].c_str());
  }
}

Mind_camera::CameraOperationCode Mind_camera::PreCapProcess() {
  MediaLibInit();

  CameraStatus status = QueryCameraStatus(config_->channel_id);
  if (status != CAMERA_STATUS_CLOSED) {
    HIAI_ENGINE_LOG("[CameraDatasets] PreCapProcess.QueryCameraStatus "
                    "{status:%d} failed.",
                    status);
    return kCameraNotClosed;
  }

  // Open Camera
  int ret = OpenCamera(config_->channel_id);
  // return 0 indicates failure
  if (ret == 0) {
    HIAI_ENGINE_LOG("[CameraDatasets] PreCapProcess OpenCamera {%d} "
                    "failed.",
                    config_->channel_id);
    return kCameraOpenFailed;
  }

  // set fps
  ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_FPS,
                          &(config_->fps));
  // return 0 indicates failure
  if (ret == 0) {
    HIAI_ENGINE_LOG("[CameraDatasets] PreCapProcess set fps {fps:%d} "
                    "failed.",
                    config_->fps);
    return kCameraSetPropertyFailed;
  }

  // set image format
  ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_IMAGE_FORMAT,
                          &(config_->image_format));
  // return 0 indicates failure
  if (ret == 0) {
    HIAI_ENGINE_LOG("[CameraDatasets] PreCapProcess set image_fromat "
                    "{format:%d} failed.",
                    config_->image_format);
    return kCameraSetPropertyFailed;
  }

  // set image resolution.
  CameraResolution resolution;
  resolution.width = config_->resolution_width;
  resolution.height = config_->resolution_height;
  ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_RESOLUTION,
                          &resolution);
  // return 0 indicates failure
  if (ret == 0) {
    HIAI_ENGINE_LOG("[CameraDatasets] PreCapProcess set resolution "
                    "{width:%d, height:%d } failed.",
                    config_->resolution_width, config_->resolution_height);
    return kCameraSetPropertyFailed;
  }

  // set work mode
  CameraCapMode mode = CAMERA_CAP_ACTIVE;
  ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_CAP_MODE, &mode);
  // return 0 indicates failure
  if (ret == 0) {
    HIAI_ENGINE_LOG("[CameraDatasets] PreCapProcess set cap mode {mode:%d}"
                    " failed.",
                    mode);
    return kCameraSetPropertyFailed;
  }

  return kCameraOk;
}

shared_ptr<AgeEstimationInfo> Mind_camera::CreateBatchImageParaObj() {
  shared_ptr<AgeEstimationInfo> pObj = make_shared<AgeEstimationInfo>();

  // handle one image frame every time
  pObj->frame.channel_id = config_->channel_id;
  pObj->frame.frame_id = frame_id_++;
  pObj->frame.timestamp = time(nullptr);

  // channel begin from zero
  pObj->org_img.channel = 0;
  pObj->org_img.format = YUV420SP;
  pObj->org_img.width = config_->resolution_width;
  pObj->org_img.height = config_->resolution_height;
  // YUV size in memory is width*height*3/2
  pObj->org_img.size = config_->resolution_width * config_->resolution_height
      * 3 / 2;

  shared_ptr<uint8_t> data(new uint8_t[pObj->org_img.size],
                           default_delete<uint8_t[]>());
  pObj->org_img.data = data;
  return pObj;
}

bool Mind_camera::DoCapProcess() {
  CameraOperationCode ret_code = PreCapProcess();
  if (ret_code == kCameraSetPropertyFailed) {
    CloseCamera(config_->channel_id);

    HIAI_ENGINE_LOG("[CameraDatasets] DoCapProcess.PreCapProcess failed");
    return false;
  }

  // set procedure is running.
  SetExitFlag(CAMERADATASETS_RUN);

  HIAI_StatusT hiai_ret = HIAI_OK;
  int read_ret = 0;
  int read_size = 0;
  bool read_flag = false;
  while (GetExitFlag() == CAMERADATASETS_RUN) {
    shared_ptr<AgeEstimationInfo> p_obj = CreateBatchImageParaObj();
    uint8_t* p_data = p_obj->org_img.data.get();
    read_size = (int) p_obj->org_img.size;

    // do read frame from camera, readSize maybe changed when called
    read_ret = ReadFrameFromCamera(config_->channel_id, (void*) p_data,
                                   &read_size);
    // indicates failure when readRet is 1
    read_flag = ((read_ret == 1) && (read_size == (int) p_obj->org_img.size));

    if (!read_flag) {
      HIAI_ENGINE_LOG("[CameraDatasets] readFrameFromCamera failed "
                      "{camera:%d, ret:%d, size:%d, expectsize:%d} ",
                      config_->channel_id, read_ret, read_size,
                      (int )p_obj->org_img.size);
      break;
    }

    hiai_ret = SendData(0, "AgeEstimationInfo",
                        static_pointer_cast<void>(p_obj));

    if (hiai_ret != HIAI_OK) {
      HIAI_ENGINE_LOG("[CameraDatasets] senddata failed! {frameid:%d, "
                      "timestamp:%lu}",
                      p_obj->frame.frame_id, p_obj->frame.timestamp);
      break;
    }
  }

  // close camera
  CloseCamera(config_->channel_id);

  if (hiai_ret != HIAI_OK) {
    return false;
  }

  return true;
}

void Mind_camera::SetExitFlag(int flag) {
  TLock lock(mutex_);
  exit_flag_ = flag;
}

int Mind_camera::GetExitFlag() {
  TLock lock(mutex_);
  return exit_flag_;
}

HIAI_IMPL_ENGINE_PROCESS("Mind_camera", Mind_camera, INPUT_SIZE) {
  clock_t start = clock();
  HIAI_ENGINE_LOG("[CameraDatasets] start process!");
  DoCapProcess();
  HIAI_ENGINE_LOG("[CameraDatasets] end process!");
  float cost=(float)(clock()-start)/CLOCKS_PER_SEC;
  HIAI_ENGINE_LOG("camera time: %f ms.", cost*1000);
  return HIAI_OK;
}

