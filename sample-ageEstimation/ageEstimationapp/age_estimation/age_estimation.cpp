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

#include "age_estimation.h"

#include <cstdint>
#include <memory>
#include <sstream>

#include<ctime>

#include "hiaiengine/log.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using hiai::Engine;
using hiai::ImageData;
using cv::Mat;
using namespace std;
using namespace ascend::utils;

namespace {
// output port (engine port begin with 0)
const uint32_t kSendDataPort = 0;

// level for call Dvpp
const int32_t kDvppToJpegLevel = 100;

// 96*112 add 40% margin
const int kResizeWidth = 224;
const int kResizeHeight = 224;

//const int finalWidth = 224;
//const int finalHeight = 224;

// image source from register
const uint32_t kRegisterSrc = 1;

// The memory size of the BGR image is 3 times that of width*height.
const int32_t kRgbBufferMultiple = 3;

// destination points for aligned face
const float kLeftEyeX = 0.39753819 * kResizeWidth;
const float kLeftEyeY = 0.39753819 * kResizeHeight;
const float kRightEyeX = 0.60145718 * kResizeWidth;
const float kRightEyeY = 0.39753819 * kResizeHeight;
const float kNoseX = 0.50014583 * kResizeWidth;
const float kNoseY = 0.57805853 * kResizeHeight;
const float kLeftMouthCornerX = 0.41637326 * kResizeWidth;
const float kLeftMouthCornerY = 0.68038442 * kResizeHeight;
const float kRightMouthCornerX = 0.58524248 * kResizeWidth;
const float kRightMouthCornerY = 0.68038442 * kResizeHeight;

// wapAffine estimate check cols(=2) and rows(=3)
const int32_t kEstimateRows = 2;
const int32_t kEstimateCols = 3;

// flip face
// Horizontally flip for OpenCV
const int32_t kHorizontallyFlip = 1;
// Vertically and Horizontally flip for OpenCV
const int32_t kVerticallyAndHorizontallyFlip = -1;

// inference batch
const int32_t kBatchSize = 1;
// every batch has one aligned face and one flip face, total 2
const int32_t kBatchImgCount = 1;
// inference result index
const int32_t kInferenceResIndex = 0;
// every face inference result should contains 101 vector (float)
const int32_t kEveryFaceResVecCount = 70;

// sleep interval when queue full (unit:microseconds)
const __useconds_t kSleepInterval = 200000;
cv::Mat frame;
}

// register custom data type
HIAI_REGISTER_DATA_TYPE("AgeEstimationInfo", AgeEstimationInfo);
HIAI_REGISTER_DATA_TYPE("FaceRectangle", FaceRectangle);
HIAI_REGISTER_DATA_TYPE("FaceImage", FaceImage);

AgeEstimation::AgeEstimation() {
  ai_model_manager_ = nullptr;
}

HIAI_StatusT AgeEstimation::Init(
    const hiai::AIConfig& config,
    const vector<hiai::AIModelDescription>& model_desc) {
  HIAI_ENGINE_LOG("Start initialize!");

  // initialize aiModelManager
  if (ai_model_manager_ == nullptr) {
    ai_model_manager_ = make_shared<hiai::AIModelManager>();
  }

  // get parameters from graph.config
  // set model path to AI model description
  hiai::AIModelDescription fg_model_desc;
  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem& item = config.items(index);
    // get model_path
    if (item.name() == kModelPathParamKey) {
      const char* model_path = item.value().data();
      fg_model_desc.set_path(model_path);
    }
    // else: noting need to do
  }

  // initialize model manager
  vector<hiai::AIModelDescription> model_desc_vec;
  model_desc_vec.push_back(fg_model_desc);
  hiai::AIStatus ret = ai_model_manager_->Init(config, model_desc_vec);
  // initialize AI model manager failed
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE, "initialize AI model failed");
    return HIAI_ERROR;
  }

  HIAI_ENGINE_LOG("End initialize!");
  return HIAI_OK;
}

bool AgeEstimation::ResizeImg(const FaceImage &face_img,
                                ImageData<u_int8_t> &resized_image) {
  // input size is less than zero, return failed
  int32_t img_size = face_img.image.size;
  if (img_size <= 0) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "image size less than or equal zero, size=%d", img_size);
    return false;
  }

  // call ez_dvpp to resize image
  DvppBasicVpcPara resize_para;
  resize_para.input_image_type = INPUT_YUV420_SEMI_PLANNER_UV;  // NV12 format
  // get original image size and set to resize parameter
  int32_t width = face_img.image.width;
  int32_t height = face_img.image.height;
  // set from 0 to width -1
  resize_para.crop_left = 0;
  resize_para.crop_right = width - 1;
  // set from 0 to height -1
  resize_para.crop_up = 0;
  resize_para.crop_down = height - 1;
  // set source resolution ratio
  resize_para.src_resolution.width = width;
  resize_para.src_resolution.height = height;
  // set destination resolution ratio
  resize_para.dest_resolution.width = kResizeWidth;
  resize_para.dest_resolution.height = kResizeHeight;
  // input already padding
  resize_para.is_input_align = true;
  // out need remove padding
  resize_para.is_output_align = false;
  // call
  DvppProcess dvpp_resize_img(resize_para);
  DvppVpcOutput dvpp_output;
  int ret = dvpp_resize_img.DvppBasicVpcProc(face_img.image.data.get(),
                                             img_size, &dvpp_output);
  if (ret != kDvppOperationOk) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "call ez_dvpp failed, failed to resize image.");
    return false;
  }

  // call success, set data and size
  resized_image.data.reset(dvpp_output.buffer, default_delete<u_int8_t[]>());
  resized_image.size = dvpp_output.size;
  resized_image.width = kResizeWidth;
  resized_image.height = kResizeHeight;
  return true;
}

bool AgeEstimation::Nv12ToRgb(const ImageData<u_int8_t> &src_image,
                                Mat &dst) {
  // transforming smart pointer data into Mat type data
  // for calling OpenCV interface
  Mat src(src_image.height * kNv12SizeMolecule / kNv12SizeDenominator,
          src_image.width, CV_8UC1);

  // number of characters to copy, the size of yuv image is 1.5 times
  // than width * height;
  int copy_size = src_image.width * src_image.height * kNv12SizeMolecule
      / kNv12SizeDenominator;

  // size of the destination buffer
  int destination_size = src.cols * src.rows * src.elemSize();

  // copy the input image data into the Mat matrix for image conversion.
  int ret = memcpy_s(src.data, destination_size, src_image.data.get(),
                     copy_size);
  // check memory_s result
  CHECK_MEM_OPERATOR_RESULTS(ret);

  // call OpenCV interface to convert image from yuv420sp_nv12 to rgb
  // OpenCV interface have no return value

  cvtColor(src, dst, CV_YUV2BGR_NV12);

  return true;
}

bool AgeEstimation::checkTransfromMat(const Mat &mat) {
  // openCV warpAffine method, need transformation matrix should match
  // 1. type need CV_32F or CV_64F
  // 2. rows must be 2
  // 3. cols must be 3
  if ((mat.type() == CV_32F || mat.type() == CV_64F)
      && mat.rows == kEstimateRows && mat.cols == kEstimateCols) {
    return true;
  }
  HIAI_ENGINE_LOG(
      "transformation matrix not match, real type=%d, rows=%d, cols=%d",
      mat.type(), mat.rows, mat.cols);
  return false;
}

bool AgeEstimation::AlignedAndFlipFace(const FaceImage &face_img,
                                       int32_t index, vector<AlignedFace> &aligned_imgs) {
  // Step1: convert NV12 to BGR
//  Mat bgr_img;
//  if (!Nv12ToBgr(resized_image, bgr_img)) {
//    // failed, no need to do anything
//    return false;
//  }

  // Step2: aligned face
  // arrange destination points
  vector<cv::Point2f> dst_points;
  dst_points.emplace_back(cv::Point2f(kLeftEyeX, kLeftEyeY));
  dst_points.emplace_back(cv::Point2f(kRightEyeX, kRightEyeY));
  dst_points.emplace_back(cv::Point2f(kNoseX, kNoseY));
  dst_points.emplace_back(cv::Point2f(kLeftMouthCornerX, kLeftMouthCornerY));
  dst_points.emplace_back(cv::Point2f(kRightMouthCornerX, kRightMouthCornerY));

  // arrange source points
  // should be changed to resized image point
  vector<cv::Point2f> src_points;
  float face_width = (float) face_img.image.width;
  float face_height = (float) face_img.image.height;
  // left eye
  float left_eye_x = face_img.feature_mask.left_eye.x;
  float left_eye_y = face_img.feature_mask.left_eye.y;
  src_points.emplace_back(left_eye_x, left_eye_y);
  // right eye
  float right_eye_x = face_img.feature_mask.right_eye.x;
  float right_eye_y = face_img.feature_mask.right_eye.y;
  src_points.emplace_back(right_eye_x, right_eye_y);
  // nose
  float nose_x = face_img.feature_mask.nose.x;
  float nose_y = face_img.feature_mask.nose.y;
  src_points.emplace_back(nose_x, nose_y);
  // left mouth corner
  float left_mouth_x = face_img.feature_mask.left_mouth.x;
  float left_mouth_y = face_img.feature_mask.left_mouth.y;
  src_points.emplace_back(left_mouth_x, left_mouth_y);
  // right mouth corner
  float right_mouth_x = face_img.feature_mask.right_mouth.x;
  float right_mouth_y = face_img.feature_mask.right_mouth.y;
  src_points.emplace_back(right_mouth_x, right_mouth_y);

  // call OpenCV to aligned
  // first set false (partialAffine), limited to combinations of translation,
  // rotation, and uniform scaling (4 degrees of freedom)
  Mat point_estimate = estimateRigidTransform(src_points, dst_points, false);

  // transform matrix not match warpAffine, set true (fullAffine)
  if (!checkTransfromMat(point_estimate)) {
    HIAI_ENGINE_LOG("estimateRigidTransform using partialAffine failed,"
                    "try to using fullAffine");
    point_estimate = estimateRigidTransform(src_points, dst_points, true);
  }

  // check again, if not match, return false
  if (!checkTransfromMat(point_estimate)) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "estimateRigidTransform using partialAffine and fullAffine all failed."
        "skip this image.");
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "left_eye=(%f, %f)",
                    left_eye_x, left_eye_y);
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "right_eye=(%f, %f)",
                    right_eye_x, right_eye_y);
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "nose=(%f, %f)", nose_x,
                    nose_y);
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "left_mouth=(%f, %f)",
                    left_mouth_x, left_mouth_y);
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "right_mouth=(%f, %f)",
                    right_mouth_x, right_mouth_y);
    return false;
  }
  cv::Size img_size(kResizeWidth, kResizeHeight);
  cv:: Mat aligned_img;
  warpAffine(frame, aligned_img, point_estimate, img_size);
//  imwrite("align.jpg",aligned_img);


  // Step4: change to RGB (because AIPP need this format)
  cvtColor(aligned_img, aligned_img, cv::COLOR_BGR2RGB);

//  aligned_img.convertTo(aligned_img, CV_32FC3);

//  uint32_t size_f = aligned_img.total() * aligned_img.channels();
//  uint32_t size_u8 = size_f*4;

//  float *nhwc = new (nothrow) float[size_f];
//  float *nchw = new (nothrow) float[size_f];
//  memcpy_s(nhwc,size_u8,aligned_img.ptr<u_int8_t>(),size_u8);
//  for (int h = 0;h < kResizeHeight; ++h)
//  {
//    for (int w = 0; w < kResizeWidth; ++w)
//    {
//      for (int c = 0; c < aligned_img.channels(); ++c)
//      {
//        //cout << h << "*" << w << "*" << c << "\t";
//        //cout << 224*224*c+224*h+w << "\t" << 224*3*w+224*h+c << endl;
//        nchw[224*224*c+224*h+w] = nhwc[224*3*h+3*w+c];
//      }
//    }
//  }
//  memcpy_s(aligned_img.ptr<u_int8_t>(),size_u8,nchw,size_u8);

//  delete[] nhwc;
//  delete[] nchw;
  AlignedFace result;
  result.aligned_face = aligned_img;

  // Step5: set back to aligned images

  result.face_index = index;

//  result.aligned_flip_face = hv_flip;
  aligned_imgs.emplace_back(result);
  return true;
}

void AgeEstimation::PreProcess(const vector<FaceImage> &face_imgs,
                                 vector<AlignedFace> &aligned_imgs) {
  // loop each cropped face image
  for (int32_t index = 0; index < face_imgs.size(); ++index) {
    // check flag, if false need not to do anything
    if (!face_imgs[index].feature_mask.flag) {
      HIAI_ENGINE_LOG("flag is false, skip it");
      continue;
    }

//    // resize image, if failed, skip it
//    ImageData<u_int8_t> resized_image;
//    if (!ResizeImg(face_imgs[index], resized_image)) {
//      continue;
//    }

    // aligned and flip face, if failed, skip it
    if (!AlignedAndFlipFace(face_imgs[index], index,
                            aligned_imgs)) {
      continue;
    }

    // success
    HIAI_ENGINE_LOG("aligned face success, index=%d", index);
  }
}

bool AgeEstimation::PrepareBuffer(int32_t batch_begin,
                                    shared_ptr<u_int8_t> &batch_buffer,
                                    uint32_t buffer_size,
                                    uint32_t each_img_size,
                                    const vector<AlignedFace> &aligned_imgs) {
  // loop for each image in one batch
  uint32_t last_size = 0;
  for (int i = 0; i < kBatchSize; ++i) {
    // real image
    if (batch_begin + i < aligned_imgs.size()) {
      AlignedFace face_img = aligned_imgs[batch_begin + i];
      // copy aligned face image
      errno_t ret = memcpy_s(batch_buffer.get() + last_size,
                             buffer_size - last_size,
                             face_img.aligned_face.ptr<u_int8_t>(),
                             each_img_size);
      // check memcpy_s result
      CHECK_MEM_OPERATOR_RESULTS(ret);
      last_size += each_img_size;

//      // copy aligned flip face image
//      ret = memcpy_s(batch_buffer.get() + last_size, buffer_size - last_size,
//                     face_img.aligned_flip_face.ptr<uint8_t>(), each_img_size);
//      // check memcpy_s result
//      CHECK_MEM_OPERATOR_RESULTS(ret);
//      last_size += each_img_size;
    } else {  // image size less than batch size
      // need set all data to 0
      errno_t ret = memset_s(batch_buffer.get() + last_size,
                             buffer_size - last_size, static_cast<char>(0),
                             each_img_size * kBatchImgCount);
      // check memset_s result
      CHECK_MEM_OPERATOR_RESULTS(ret);
      last_size += each_img_size * kBatchImgCount;
    }
  }

  return true;
}

bool AgeEstimation::ArrangeResult(
    int32_t batch_begin,
    const vector<shared_ptr<hiai::IAITensor>> &output_data_vec,
    const vector<AlignedFace> &aligned_imgs, vector<FaceImage> &face_imgs) {

  // Step1: get every batch inference result, in first vector
  shared_ptr<hiai::AINeuralNetworkBuffer> inference_result =
      static_pointer_cast<hiai::AINeuralNetworkBuffer>(
          output_data_vec[kInferenceResIndex]);
  int32_t inference_size = inference_result->GetSize() / sizeof(float);

  // Step2: size check
  // (1) get real face batch count
  int32_t batch_count = kBatchSize;
  if (aligned_imgs.size() - batch_begin < kBatchSize) {
    batch_count = aligned_imgs.size() - batch_begin;
  }

  // (2) every image need 1024 float vector
  int32_t check_size = batch_count * kEveryFaceResVecCount;
  if (check_size > inference_size) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "inference result size not correct. ",
        "batch_begin=%d, total_face=%d, inference_size=%d, need_size=%d",
        batch_begin, aligned_imgs.size(), inference_size, check_size);
    return false;
  }

  // Step3: copy results
  float result[inference_size];
  errno_t ret = memcpy_s(result, sizeof(result), inference_result->GetBuffer(),
                         inference_result->GetSize());
  // check memcpy_s result
  CHECK_MEM_OPERATOR_RESULTS(ret);

  // Step4: set every face inference result
  for (int i = 0; i < batch_count; ++i) {
    // set to face_imgs according to index
    int32_t org_crop_index = aligned_imgs[batch_begin + i].face_index;
    float age = 0;
    for (int j = 0; j < kEveryFaceResVecCount; ++j) {
      int32_t result_index = i * kEveryFaceResVecCount + j;
//      face_imgs[org_crop_index].feature_vector.emplace_back(
//          result[result_index]);
      age+=j*result[result_index];
    }
    face_imgs[org_crop_index].age = age;
    HIAI_ENGINE_LOG("set inference result successfully. face index=%d",
                    i + batch_begin);
  }

  return true;
}

void AgeEstimation::InferenceFeatureVector(
    const vector<AlignedFace> &aligned_imgs, vector<FaceImage> &face_imgs) {
  // initialize buffer
  //size of float = 4
  uint32_t each_img_size = aligned_imgs[0].aligned_face.total()
      * aligned_imgs[0].aligned_face.channels();
  uint32_t buffer_size = each_img_size * kBatchImgCount * kBatchSize;
  shared_ptr<u_int8_t> batch_buffer = shared_ptr<u_int8_t>(
      new u_int8_t[buffer_size], default_delete<u_int8_t[]>());

  // loop for each batch
  for (int32_t index = 0; index < aligned_imgs.size(); index += kBatchSize) {
    // 1. prepare input buffer for each batch
    // if prepare failed, also need to deal next batch
    if (!PrepareBuffer(index, batch_buffer, buffer_size, each_img_size,
                       aligned_imgs)) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "prepare buffer failed, batch_begin=%d, batch_size=%d",
                      index, kBatchSize);
      continue;
    }

    // 2. prepare input data
    shared_ptr<hiai::AINeuralNetworkBuffer> neural_buf = shared_ptr<
        hiai::AINeuralNetworkBuffer>(
        new hiai::AINeuralNetworkBuffer(),
        default_delete<hiai::AINeuralNetworkBuffer>());
    neural_buf->SetBuffer((void*) batch_buffer.get(), buffer_size);
    shared_ptr<hiai::IAITensor> input_data =
        static_pointer_cast<hiai::IAITensor>(neural_buf);
    vector<shared_ptr<hiai::IAITensor>> input_data_vec;
    input_data_vec.emplace_back(input_data);

    // 3. create output tensor
    hiai::AIContext ai_context;
    vector<shared_ptr<hiai::IAITensor>> output_data_vec;
    hiai::AIStatus ret = ai_model_manager_->CreateOutputTensor(input_data_vec,
                                                               output_data_vec);
    // create failed, need to deal next batch
    if (ret != hiai::SUCCESS) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "failed to create output tensor, ret=%u", ret);
      continue;
    }

    // 4. Process inference

    HIAI_ENGINE_LOG("begin to call AI Model Manager Process method.");
    ret = ai_model_manager_->Process(ai_context, input_data_vec,
                                     output_data_vec,
                                     AI_MODEL_PROCESS_TIMEOUT);
    // process failed, need to deal next batch
    if (ret != hiai::SUCCESS) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "failed to process AI Model, ret=%u", ret);
      continue;
    }
    HIAI_ENGINE_LOG("end to call AI Model Manager Process method.");


    // 5. arrange result for each batch
    // if failed, also need to deal next batch
    if (!ArrangeResult(index, output_data_vec, aligned_imgs, face_imgs)) {
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "arrange inference result failed, batch_begin=%d, batch_size=%d",
          index, kBatchSize);
      continue;
    }
  }
}

bool AgeEstimation::GetOriginPic(
  const shared_ptr<AgeEstimationInfo> &image_handle) {
  if(image_handle->frame.image_source == 0) {
    HIAI_ENGINE_LOG("Begin to transfer the image to jpeg.");

    // Generate frame information
    // JPEG image (call DVPP change NV12 to jpeg)
    DvppToJpgPara dvpp_to_jpeg_para;
    dvpp_to_jpeg_para.format = JPGENC_FORMAT_NV12;
    dvpp_to_jpeg_para.level = kDvppToJpegLevel;
    dvpp_to_jpeg_para.resolution.height = image_handle->org_img.height;
    dvpp_to_jpeg_para.resolution.width = image_handle->org_img.width;
    DvppProcess dvpp_to_jpeg(dvpp_to_jpeg_para);
    DvppOutput dvpp_output;
    int32_t ret = dvpp_to_jpeg.DvppOperationProc(
                    reinterpret_cast<char*>(image_handle->org_img.data.get()), image_handle->org_img.size,
                    &dvpp_output);

    // failed, no need to send to presenter
    if(ret != kDvppOperationOk) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Failed to convert NV12 to JPEG, skip this frame.");
      return false;
    }

    image_handle->frame.original_jpeg_pic_buffer = dvpp_output.buffer;
    image_handle->frame.original_jpeg_pic_size = dvpp_output.size;
  }
  return true;
}

void AgeEstimation::SendResult(
    const shared_ptr<AgeEstimationInfo> &image_handle) {
  HIAI_StatusT hiai_ret;
  if(!GetOriginPic(image_handle)){
    image_handle->err_info.err_code = AppErrorCode::kRecognition;
    image_handle->err_info.err_msg = "Get the original pic failed";

    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Engine handle filed, err_code=%d, err_msg=%s",
                    image_handle->err_info.err_code,
                    image_handle->err_info.err_msg.c_str());
  }
  // when register face, can not discard when queue full
  do {
    hiai_ret = SendData(0, "AgeEstimationInfo",
                        static_pointer_cast<void>(image_handle));
    // when queue full, sleep
    if (hiai_ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG("queue full, sleep 200ms");
      usleep(kSleepInterval);
    }
  } while (hiai_ret == HIAI_QUEUE_FULL
      && image_handle->frame.image_source == kRegisterSrc);

  // send failed
  if (hiai_ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "call SendData failed, err_code=%d", hiai_ret);
  }
}

HIAI_StatusT AgeEstimation::Recognition(
    shared_ptr<AgeEstimationInfo> &image_handle) {
  string err_msg = "";
  if (image_handle->err_info.err_code != AppErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "front engine dealing failed, err_code=%d, err_msg=%s",
                    image_handle->err_info.err_code,
                    image_handle->err_info.err_msg.c_str());
    SendResult(image_handle);
    return HIAI_ERROR;
  }

  // pre-process
  vector<AlignedFace> aligned_imgs;
  PreProcess(image_handle->face_imgs, aligned_imgs);

  // need to inference or not
  if (aligned_imgs.empty()) {
    HIAI_ENGINE_LOG("no need to inference any image.");
    SendResult(image_handle);
    return HIAI_OK;
  }

  // inference and set results
  InferenceFeatureVector(aligned_imgs, image_handle->face_imgs);

  // send result
  SendResult(image_handle);
  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("age_estimation",
    AgeEstimation, INPUT_SIZE) {
  HIAI_StatusT ret = HIAI_OK;
  clock_t start = clock();
  // deal arg0 (engine only have one input)
  if (arg0 != nullptr) {
    HIAI_ENGINE_LOG("begin to deal face_recognition!");
    shared_ptr<AgeEstimationInfo> image_handle = static_pointer_cast<
        AgeEstimationInfo>(arg0);
    Nv12ToRgb(image_handle->org_img, frame);
    ret = Recognition(image_handle);
  }
  float cost=(float)(clock()-start)/CLOCKS_PER_SEC;
  HIAI_ENGINE_LOG("age estimation time: %f ms.", cost*1000);
  return ret;
}
