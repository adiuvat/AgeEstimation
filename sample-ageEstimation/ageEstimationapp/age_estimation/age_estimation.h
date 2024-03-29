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

#ifndef FACE_RECOGNITION_ENGINE_H_
#define FACE_RECOGNITION_ENGINE_H_

#include <vector>

#include "hiaiengine/api.h"
#include "hiaiengine/ai_model_manager.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/engine.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/ai_tensor.h"
#include "opencv2/opencv.hpp"

#include "age_estimation_params.h"

#define INPUT_SIZE 2
#define OUTPUT_SIZE 1

#define AI_MODEL_PROCESS_TIMEOUT 0

// aligned face data
struct AlignedFace {
// face index (using for set result)
  int32_t face_index;
// aligned face
  cv::Mat aligned_face;
// flip face according to aligned face
  cv::Mat aligned_flip_face;
};

/**
 * @brief: inference engine class
 */
class AgeEstimation : public hiai::Engine {
public:
  /**
   * @brief: construction function
   */
  AgeEstimation();

  /**
   * @brief: destruction function
   */
  ~AgeEstimation() = default;

  /**
   * @brief: face detection inference engine initialize
   * @param [in]: engine's parameters which configured in graph.config
   * @param [in]: model description
   * @return: HIAI_StatusT
   */
  HIAI_StatusT Init(const hiai::AIConfig& config,
                    const std::vector<hiai::AIModelDescription>& model_desc);

  /**
   * @brief: engine processor which override HIAI engine
   *         inference every image, and then send data to post process
   * @param [in]: input size
   * @param [in]: output size
   */
HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)
  ;

private:
  int cnt;


// cache AI model parameters
  std::shared_ptr<hiai::AIModelManager> ai_model_manager_;

  /**
   * @brief: pre-process
   * param [in]: face_imgs: face images
   * param [out]: aligned_imgs: aligned output images (RGB)
   */
  void PreProcess(const std::vector<FaceImage> &face_imgs,
                  std::vector<AlignedFace> &aligned_imgs);
  /**
   * @brief: resize image (already padding)
   * param [in]: face_img: cropped face image
   * param [out]: resized_image: call ez_dvpp output image
   * @return: true: success; false: failed
   */
  bool ResizeImg(const FaceImage &face_img,
                 hiai::ImageData<u_int8_t> &resized_image);

  /**
   * @brief Image format conversion, call OpenCV interface to transform
   *        the image, from YUV420SP_NV12 to BGR
   * @param [in] src_image: source image
   * @param [out] dst: image after conversion, Mat type
   * @return true: yuv420spnv12 convert to BGR success
   *         false: yuv420spnv12 convert to BGR failed
   */
  bool Nv12ToRgb(const hiai::ImageData<u_int8_t> &src_image, cv::Mat &dst);

  /**
   * @brief check transformation matrix for openCV wapAffine
   * @param [in] mat: transformation matrix
   * @return true: match
   *         false: not match
   */
  bool checkTransfromMat(const cv::Mat &mat);

  /**
   * @brief: aligned and flip face
   * param [in]: face_img: cropped face image
   * param [in]: resized_image: call ez_dvpp output image
   * param [in]: index: image index
   * param [out]: aligned_imgs: result image
   * @return: true: success; false: failed
   */
  bool AlignedAndFlipFace(const FaceImage &face_img,
                          int32_t index,
                          std::vector<AlignedFace> &aligned_imgs);

  /**
   * @brief: prepare batch buffer
   * param [in]: batch_begin: batch begin index
   * param [in]: img_count: total face count
   * param [out]: batch_buffer: batch buffer
   * param [in]: buffer_size: batch buffer total size
   * param [in]: each_img_size: each face image size
   * param [in]: aligned_imgs: aligned face and flip images
   * @return: true: success; false: failed
   */
  bool PrepareBuffer(int32_t batch_begin,
                     std::shared_ptr<u_int8_t> &batch_buffer,
                     uint32_t buffer_size, uint32_t each_img_size,
                     const std::vector<AlignedFace> &aligned_imgs);

  /**
   * @brief: prepare batch buffer
   * param [in]: batch_begin: batch begin index
   * param [in]: output_data_vec: inference output data for each batch
   * param [in]: aligned_imgs: aligned face and flip images
   * param [out]: face_imgs: face images
   * @return: true: success; false: failed
   */
  bool ArrangeResult(
      int32_t batch_begin,
      const std::vector<std::shared_ptr<hiai::IAITensor>> &output_data_vec,
      const std::vector<AlignedFace> &aligned_imgs,
      std::vector<FaceImage> &face_imgs);

  /**
   * @brief: inference
   * param [in]: aligned_imgs: aligned face images
   * param [out]: face_imgs: face images
   */
  void InferenceFeatureVector(const std::vector<AlignedFace> &aligned_imgs,
                              std::vector<FaceImage> &face_imgs);

  /**
   * @brief: face recognition
   * @param [out]: original information from front-engine
   * @return: HIAI_StatusT
   */
  HIAI_StatusT Recognition(std::shared_ptr<AgeEstimationInfo> &image_handle);

  /**
   * @brief: send result
   * param [out]: image_handle: engine transform data
   */
  void SendResult(const std::shared_ptr<AgeEstimationInfo> &image_handle);

  /**
   * @brief: Get original picture from camera
   * param [out]: image_handle: engine transform data
   */
  bool GetOriginPic(const std::shared_ptr<AgeEstimationInfo> &image_handle);
};

#endif /* FACE_RECOGNITION_ENGINE_H_ */
