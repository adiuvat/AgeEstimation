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
#ifndef FACE_DETECTION_PARAMS_H_
#define FACE_DETECTION_PARAMS_H_

#include "hiaiengine/data_type.h"
#include "ascenddk/ascend_ezdvpp/dvpp_data_type.h"

#define CHECK_MEM_OPERATOR_RESULTS(ret) \
if (ret != EOK) { \
  HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, \
                  "memory operation failed, error=%d", ret); \
  return false; \
}

// NV12 image's transformation param
// The memory size of the NV12 image is 1.5 times that of width*height.
const int32_t kNv12SizeMolecule = 3;
const int32_t kNv12SizeDenominator = 2;

// model path parameter key in graph.config
const string kModelPathParamKey = "model_path";

// batch size parameter key in graph.config
const string kBatchSizeParamKey = "batch_size";

/**
 * @brief: face recognition APP error code definition
 */
enum class AppErrorCode {
  // Success, no error
  kNone = 0,

  // register engine failed
  kRegister,

  // detection engine failed
  kDetection,

  // feature mask engine failed
  kFeatureMask,

  // recognition engine failed
  kRecognition
};

/**
 * @brief: frame information
 */
struct FrameInfo {
  uint32_t frame_id = 0;  // frame id
  uint32_t channel_id = 0;  // channel id for current frame
  uint32_t timestamp = 0;  // timestamp for current frame
  uint32_t image_source = 0;  // 0:Camera 1:Register
  std::string face_id = "";  // registered face id
  // original image format and rank using for org_img addition
  // IMAGEFORMAT defined by HIAI engine does not satisfy the dvpp condition
  VpcInputFormat org_img_format = INPUT_YUV420_SEMI_PLANNER_UV;
  bool img_aligned = false; // original image already aligned or not
  unsigned char *original_jpeg_pic_buffer; // ouput buffer
  unsigned int original_jpeg_pic_size; // size of output buffer
};

/**
 * @brief: serialize for FrameInfo
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FrameInfo& data) {
  ar(data.frame_id, data.channel_id, data.timestamp, data.image_source,
     data.face_id, data.org_img_format, data.img_aligned);
}

/**
 * @brief: Error information
 */
struct ErrorInfo {
  AppErrorCode err_code = AppErrorCode::kNone;
  std::string err_msg = "";
};

/**
 * @brief: serialize for ErrorInfo
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, ErrorInfo& data) {
  ar(data.err_code, data.err_msg);
}

/**
 * @brief: custom data type: ScaleInfo
 */
struct ScaleInfoT {
  float scale_width = 1;
  float scale_height = 1;
};

template<class Archive>
void serialize(Archive& ar, ScaleInfoT& data) {
  ar(data.scale_width, data.scale_height);
}

/**
 * @brief: custom data type: NewImagePara
 */
struct NewImageParaT {
  hiai::FrameInfo f_info;
  hiai::ImageData<u_int8_t> img;
  ScaleInfoT scale_info;
};

template<class Archive>
void serialize(Archive& ar, NewImageParaT& data) {
  ar(data.f_info, data.img, data.scale_info);
}

/**
 * @brief: custom data type: NewImagePara2
 */
struct NewImageParaT2 {
  hiai::FrameInfo f_info;
  hiai::ImageData<float> img;
  ScaleInfoT scale_info;
};

template<class Archive>
void serialize(Archive& ar, NewImageParaT2& data) {
  ar(data.f_info, data.img, data.scale_info);
}

/**
 * @brief: custom data type: BatchImageParaWithScale
 */
struct BatchImageParaWithScaleT {
  hiai::BatchInfo b_info;
  std::vector<NewImageParaT> v_img;
};

template<class Archive>
void serialize(Archive& ar, BatchImageParaWithScaleT& data) {
  ar(data.b_info, data.v_img);
}

/**
 * @brief: custom data type: ImageAll
 */
struct ImageAll {
  int width_org;
  int height_org;
  int channal_org;
  hiai::ImageData<float> image;
};

template<class Archive>
void serialize(Archive& ar, ImageAll& data) {
  ar(data.width_org, data.height_org, data.channal_org, data.image);
}

/**
 * @brief: custom data type: BatchImageParaScale
 */
struct BatchImageParaScale {
  hiai::BatchInfo b_info;             // batch信息
  std::vector<ImageAll> v_img;  // batch中的图像
};

template<class Archive>
void serialize(Archive& ar, BatchImageParaScale& data) {
  ar(data.b_info, data.v_img);
}

/**
 * @brief: custom data type: OutputT
 *         defined for results output data
 */
struct OutputT {
  int32_t size;
  std::shared_ptr<u_int8_t> data;
};

/**
 * @brief: serialize for OutputT
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, OutputT& data) {
  ar(cereal::binary_data(data.data.get(), data.size * sizeof(u_int8_t)));
}

/**
 * @brief: custom data type: EngineTransT
 *         defined for face detection engine and age estimation engine
 */
struct EngineTransT {
  bool status;
  std::string msg;  // error message
  hiai::BatchInfo b_info;
  std::vector<OutputT> output_datas;
  std::vector<NewImageParaT> imgs;
};

/**
 * @brief: serialize for EngineTransT
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, EngineTransT& data) {
  ar(data.status, data.msg, data.b_info, data.output_datas, data.imgs);
}

/**
 * @brief: custom data type: EngineTransT2
 *         defined for age estimation engine and post process engine
 */
struct EngineTransT2 {
  bool status;
  std::string msg;  // error message
  hiai::BatchInfo b_info;
  std::vector<OutputT> output_datas;
  std::vector<NewImageParaT> imgs;
  std::vector<std::vector<float>> ages;
};

/**
 * @brief: serialize for EngineTransT2
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, EngineTransT2& data) {
  ar(data.status, data.msg, data.b_info, data.output_datas, data.imgs, data.ages);
}



/**
 * @brief: face rectangle
 */
struct FaceRectangle {
  hiai::Point2D lt;  // left top
  hiai::Point2D rb;  // right bottom
};

/**
 * @brief: serialize for FacePoint
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FaceRectangle& data) {
  ar(data.lt, data.rb);
}



struct AgeResult {
    int x1;
    int y1;
    int x2;
    int y2;
    float score;
    float age;
};

/**
 * @brief: face feature
 */
struct FaceFeature {
  bool flag;
  hiai::Point2D left_eye;  // left eye
  hiai::Point2D right_eye;  // right eye
  hiai::Point2D nose;  // nose
  hiai::Point2D left_mouth;  // left mouth
  hiai::Point2D right_mouth;  // right mouth
};

/**
 * @brief: serialize for FaceFeature
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FaceFeature& data) {
  ar(data.left_eye, data.right_eye, data.nose, data.left_mouth,
     data.right_mouth);
}

/**
 * @brief: face image
 */
struct FaceImage {
  hiai::ImageData<u_int8_t> image;  // cropped image from original image
  FaceRectangle rectangle;  // face rectangle
  float score;
  FaceFeature feature_mask;  // face feature mask
  float age;
};

/**
 * @brief: serialize for FaceImage
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, FaceImage& data) {
  ar(data.image, data.rectangle, data.score, data.feature_mask, data.age);
}

/**
 * @brief: information for face recognition
 */
struct AgeEstimationInfo {
  FrameInfo frame;  // frame information
  ErrorInfo err_info;  // error information
  hiai::ImageData<u_int8_t> org_img;  // original image
  std::vector<FaceImage> face_imgs;  // cropped image
};

/**
 * @brief: serialize for AgeEstimationInfo
 *         engine uses it to transfer data between host and device
 */
template<class Archive>
void serialize(Archive& ar, AgeEstimationInfo& data) {
  ar(data.frame, data.err_info, data.org_img, data.face_imgs);
}




#endif /* FACE_DETECTION_PARAMS_H_ */
