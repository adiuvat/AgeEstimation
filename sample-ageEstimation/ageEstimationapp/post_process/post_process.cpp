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
#include "post_process.h"
#include <vector>
#include <sstream>
#include <cmath>
#include <regex>
#include <cstdlib>
#include <limits>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include "hiaiengine/log.h"

using hiai::Engine;
//using namespace ascend::presenter;
using namespace std::__cxx11;


// register data type
HIAI_REGISTER_DATA_TYPE("AgeEstimationInfo", AgeEstimationInfo);
HIAI_REGISTER_DATA_TYPE("FrameInfo", FrameInfo);
HIAI_REGISTER_DATA_TYPE("FaceImage", FaceImage);
// constants
namespace {
//// parameters for drawing box and label begin////
// face box color
const uint8_t kFaceBoxColorR = 255;
const uint8_t kFaceBoxColorG = 190;
const uint8_t kFaceBoxColorB = 0;

// face box border width
const int kFaceBoxBorderWidth = 2;

// face label color
const uint8_t kFaceLabelColorR = 255;
const uint8_t kFaceLabelColorG = 255;
const uint8_t kFaceLabelColorB = 0;

// face label font
const double kFaceLabelFontSize = 0.7;
const int kFaceLabelFontWidth = 2;

// face label text prefix
const std::string kFaceLabelTextPrefix = "Age:";
//// parameters for drawing box and label end////

// port number range
const int32_t kPortMinNumber = 0;
const int32_t kPortMaxNumber = 65535;

// confidence range
const float kConfidenceMin = 0.0;
const float kConfidenceMax = 1.0;

// face detection function return value
const int32_t kFdFunSuccess = 0;
const int32_t kFdFunFailed = -1;

// need to deal results when index is 2
const int32_t kDealResultIndex = 2;

// each results size
const int32_t kEachResultSize = 7;

// attribute index
const int32_t kAttributeIndex = 1;

// score index
const int32_t kScoreIndex = 2;

// anchor_lt.x index
const int32_t kAnchorLeftTopAxisIndexX = 3;

// anchor_lt.y index
const int32_t kAnchorLeftTopAxisIndexY = 4;

// anchor_rb.x index
const int32_t kAnchorRightBottomAxisIndexX = 5;

// anchor_rb.y index
const int32_t kAnchorRightBottomAxisIndexY = 6;

// face attribute
const float kAttributeFaceLabelValue = 1.0;
const float kAttributeFaceDeviation = 0.00001;

// percent
const int32_t kScorePercent = 100;

// IP regular expression
const std::string kIpRegularExpression =
    "^((25[0-5]|2[0-4]\\d|[1]{1}\\d{1}\\d{1}|[1-9]{1}\\d{1}|\\d{1})($|(?!\\.$)\\.)){4}$";

// channel name regular expression
const std::string kChannelNameRegularExpression = "[a-zA-Z0-9/]+";
}

PostProcess::PostProcess() {
  fd_post_process_config_ = nullptr;
//  presenter_channel_ = nullptr;
}

HIAI_StatusT PostProcess::Init(
    const hiai::AIConfig& config,
    const std::vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG("Begin initialize!");

  // get configurations
  if (fd_post_process_config_ == nullptr) {
    fd_post_process_config_ = std::make_shared<PostConfig>();
  }

  // get parameters from graph.config
  for (int index = 0; index < config.items_size(); index++) {
    const ::hiai::AIConfigItem& item = config.items(index);
    const std::string& name = item.name();
    const std::string& value = item.value();
    std::stringstream ss;
    ss << value;
    if (name == "Confidence") {
      ss >> (*fd_post_process_config_).confidence;
      // validate confidence
      if (IsInvalidConfidence(fd_post_process_config_->confidence)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "Confidence=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
    } else if (name == "PresenterIp") {
      // validate presenter server IP
      if (IsInValidIp(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "PresenterIp=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      ss >> (*fd_post_process_config_).presenter_ip;
    } else if (name == "PresenterPort") {
      ss >> (*fd_post_process_config_).presenter_port;
      // validate presenter server port
      if (IsInValidPort(fd_post_process_config_->presenter_port)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "PresenterPort=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
    } else if (name == "ChannelName") {
      // validate channel name
      if (IsInValidChannelName(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "ChannelName=%s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      ss >> (*fd_post_process_config_).channel_name;
    }
    // else : nothing need to do

  }

  // call presenter agent, create connection to presenter server
  uint16_t u_port = static_cast<uint16_t>(fd_post_process_config_
      ->presenter_port);
//  OpenChannelParam channel_param = { fd_post_process_config_->presenter_ip,
//      u_port, fd_post_process_config_->channel_name, ContentType::kVideo };
//  Channel *chan = nullptr;
//  PresenterErrorCode err_code = OpenChannel(chan, channel_param);
//  // open channel failed
//  if (err_code != PresenterErrorCode::kNone) {
//    HIAI_ENGINE_LOG(HIAI_GRAPH_INIT_FAILED,
//                    "Open presenter channel failed, error code=%d", err_code);
//    return HIAI_ERROR;
//  }

//  presenter_channel_.reset(chan);
//  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "End initialize!");
  socket_fd = socket(AF_INET, SOCK_STREAM,0);
  if(socket_fd == -1)
  {
      cout<<"socket 创建失败："<<endl;
      exit(-1);
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(u_port);
  addr.sin_addr.s_addr = inet_addr(fd_post_process_config_->presenter_ip.c_str());

  int res = connect(socket_fd,(struct sockaddr*)&addr,sizeof(addr));
  if(res == -1)
  {
      cout<<"bind 链接失败："<<endl;
      exit(-1);
  }
  cout<<"bind 链接成功："<<endl;
  return HIAI_OK;
}

bool PostProcess::IsInValidIp(const std::string &ip) {
  regex re(kIpRegularExpression);
  smatch sm;
  return !regex_match(ip, sm, re);
}

bool PostProcess::IsInValidPort(int32_t port) {
  return (port <= kPortMinNumber) || (port > kPortMaxNumber);
}

bool PostProcess::IsInValidChannelName(
    const std::string &channel_name) {
  regex re(kChannelNameRegularExpression);
  smatch sm;
  return !regex_match(channel_name, sm, re);
}

bool PostProcess::IsInvalidConfidence(float confidence) {
  return (confidence <= kConfidenceMin) || (confidence > kConfidenceMax);
}

//bool PostProcess::IsInvalidResults(float attr, float score,
//                                                const Point &point_lt,
//                                                const Point &point_rb) {
//  // attribute is not face (background)
//  if (std::abs(attr - kAttributeFaceLabelValue) > kAttributeFaceDeviation) {
//    return true;
//  }

//  // confidence check
//  if ((score < fd_post_process_config_->confidence)
//      || IsInvalidConfidence(score)) {
//    return true;
//  }

//  // rectangle position is a point or not: lt == rb
//  if ((point_lt.x == point_rb.x) && (point_lt.y == point_rb.y)) {
//    return true;
//  }
//  return false;
//}

int32_t PostProcess::SendImage(uint32_t size, u_int8_t *data, std::vector<AgeResult>& age_results) {
  int32_t status = kFdFunSuccess;
  // parameter
//  ImageFrame image_frame_para;
//  image_frame_para.format = ImageFormat::kJpeg;
//  image_frame_para.width = width;
//  image_frame_para.height = height;
//  image_frame_para.size = size;
//  image_frame_para.data = data;
//  image_frame_para.detection_results = detection_results;

//  PresenterErrorCode p_ret = PresentImage(presenter_channel_.get(),
//                                            image_frame_para);
//  // send to presenter failed
//  if (p_ret != PresenterErrorCode::kNone) {
//    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
//                      "Send JPEG image to presenter failed, error code=%d",
//                      p_ret);
//    status = kFdFunFailed;
//  }
  char face_info_str[1024];
  memset(face_info_str, 0, sizeof(face_info_str));
  int pos = 0;
  pos += sprintf(face_info_str+pos, "%d:",size);
  for(int i=0;i<age_results.size();i++){
    pos += sprintf(face_info_str+pos, "%d,",age_results[i].x1);
    pos += sprintf(face_info_str+pos, "%d,",age_results[i].y1);
    pos += sprintf(face_info_str+pos, "%d,",age_results[i].x2);
    pos += sprintf(face_info_str+pos, "%d,",age_results[i].y2);
    pos += sprintf(face_info_str+pos, "%.2f,",age_results[i].age);
    pos += sprintf(face_info_str+pos, "%.2f;",age_results[i].score);
  }
  char return_info;
  send(socket_fd,face_info_str,1024,0);
//  send(socket_fd, &size, sizeof(size),0);
  recv(socket_fd, &return_info,sizeof(return_info),0);
  if(return_info=='A'){
      send(socket_fd,data,size,0);
      recv(socket_fd, &return_info,sizeof(return_info),0);
      if(return_info!='B')
          status = kFdFunFailed;
  }
  else
    status = kFdFunFailed;

  return status;
}

//HIAI_StatusT PostProcess::HandleOriginalImage(
//    const std::shared_ptr<EngineTransT2> &inference_res) {
//  HIAI_StatusT status = HIAI_OK;
//  std::vector<NewImageParaT> img_vec = inference_res->imgs;
//  // dealing every original image
//  for (uint32_t ind = 0; ind < inference_res->b_info.batch_size; ind++) {
//    uint32_t width = img_vec[ind].img.width;
//    uint32_t height = img_vec[ind].img.height;
//    uint32_t size = img_vec[ind].img.size;

//    // call SendImage
//    // 1. call DVPP to change YUV420SP image to JPEG
//    // 2. send image to presenter
//    vector<AgeResult> age_results;
//    int32_t ret = SendImage(height, width, size, img_vec[ind].img.data.get(), age_results);
//    if (ret == kFdFunFailed) {
//      status = HIAI_ERROR;
//      continue;
//    }
//  }
//  return status;
//}



HIAI_StatusT PostProcess::HandleResults(
    const std::shared_ptr<AgeEstimationInfo> &image_handle) {
  HIAI_StatusT status = HIAI_OK;

  //TODO: send the age info to the presenter
  std::vector<FaceImage> face_images = image_handle->face_imgs;
  std::vector<AgeResult> age_results;
  for(int i=0;i<face_images.size();i++){
      AgeResult one_result;
      one_result.age = face_images[i].age;
      one_result.x1 = face_images[i].rectangle.lt.x;
      one_result.y1 = face_images[i].rectangle.lt.y;
      one_result.x2 = face_images[i].rectangle.rb.x;
      one_result.y2 = face_images[i].rectangle.rb.y;
      one_result.score = face_images[i].score;
      age_results.push_back(one_result);
  }


  int32_t ret;
  ret = SendImage(image_handle->frame.original_jpeg_pic_size,
                  image_handle->frame.original_jpeg_pic_buffer, age_results);

  // check send result
  if (ret == kFdFunFailed) {
     status = HIAI_ERROR;
  }
  return status;
}

HIAI_IMPL_ENGINE_PROCESS("post_process",
    PostProcess, INPUT_SIZE) {
    clock_t start = clock();
  // check arg0 is null or not
  if (arg0 == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Failed to process invalid message.");
    return HIAI_ERROR;
  }
  shared_ptr<AgeEstimationInfo> image_handle = static_pointer_cast<
      AgeEstimationInfo>(arg0);
//  // check original image is empty or not
//  std::shared_ptr<EngineTransT2> inference_res = std::static_pointer_cast<
//      EngineTransT2>(arg0);
//  if (inference_res->imgs.empty()) {
//    HIAI_ENGINE_LOG(
//        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
//        "Failed to process invalid message, original image is null.");
//    return HIAI_ERROR;
//  }

//  // inference failed, dealing original images
//  if (!inference_res->status) {
//    HIAI_ENGINE_LOG(HIAI_OK, inference_res->msg.c_str());
//    HIAI_ENGINE_LOG(HIAI_OK, "will handle original image.");
//    return HandleOriginalImage(inference_res);
//  }

  // inference success, dealing inference results
  HIAI_StatusT ret = HandleResults(image_handle);
  float cost=(float)(clock()-start)/CLOCKS_PER_SEC;
  HIAI_ENGINE_LOG("post process time: %f ms.", cost*1000);
  return ret;
}
