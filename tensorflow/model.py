rd n3
import tensorflow as tf
from PIL import Image
import numpy as np


from tensorflow.contrib.slim import nets, initializers, init_ops
from mobilenet_v1 import mobilenet_v1,mobilenet_v1_arg_scope
import preprocess
slim = tf.contrib.slim
_R_MEAN = 123.68
_G_MEAN = 116.78
_B_MEAN = 103.94


class Model(object):
    """xxx definition."""

    def __init__(self):
        self._fixed_resize_side = 256
        self._default_image_size = 224





    def preprocess(self, image, is_training):
        """preprocessing.

        Outputs of this function can be passed to loss or postprocess functions.

        Args:
            preprocessed_inputs: A float32 tensor with shape [batch_size,
                height, width, num_channels] representing a batch of images.

        Returns:
            prediction_dict: A dictionary holding prediction tensors to be
                passed to the Loss or Postprocess functions.
        """

        def preprocess_train(image):
            image = preprocess.preprocess_images(
                image, self._default_image_size, self._default_image_size,
                is_training=True, fast_mode=False, crop_image=False)
            return image

        def preprocess_eval(image):
            # image = preprocess.preprocess_images(
            #     image, self._default_image_size, self._default_image_size,
            #     is_training=False, crop_image=False)
            image = tf.subtract(image, 0.5)
            image = tf.multiply(image, 2.0)
            return image

        image = tf.div(image, 255.0)
        outputs = tf.cond(is_training, lambda: preprocess_train(image), lambda: preprocess_eval(image))

        return outputs

    def predict(self, preprocessed_inputs, is_training):
        """Predict prediction tensors from inputs tensor.

        Outputs of this function can be passed to loss or postprocess functions.

        Args:
            preprocessed_inputs: A float32 tensor with shape [batch_size,
                height, width, num_channels] representing a batch of images.

        Returns:
            prediction_dict: A dictionary holding prediction tensors to be
                passed to the Loss or Postprocess functions.
        """
        with slim.arg_scope(mobilenet_v1_arg_scope(is_training=is_training)):
            logits, end_points = mobilenet_v1(preprocessed_inputs, is_training=is_training, depth_multiplier=1.0, num_classes=70)


        prob = tf.nn.softmax(logits, name='prob')
        prediction_dict = {'prob': prob}
        return prediction_dict

    def postprocess(self, prediction_dict):
        """Convert predicted output tensors to final forms.

        Args:
            prediction_dict: A dictionary holding prediction tensors.
            **params: Additional keyword arguments for specific implementations
                of specified models.

        Returns:
            A dictionary containing the postprocessed results.
        """
        prob = prediction_dict['prob']
        rank = [i for i in range(70)]
        age = tf.reduce_sum(prob*rank, 1)

        postprocessed_dict = {'age': age}
        return postprocessed_dict

    def loss(self, prediction_dict, groundtruth_lists):
        """Compute scalar loss tensors with respect to provided groundtruth.

        Args:
            prediction_dict: A dictionary holding prediction tensors.
            groundtruth_lists_dict: A dict of tensors holding groundtruth
                information, with one entry for each image in the batch.

        Returns:
            A dictionary mapping strings (loss names) to scalar tensors
                representing loss values.
                :param groundtruth_lists:
        """
        prob = prediction_dict['prob']
        # prob += 1e-10
        # groundtruth_lists += 1e-10
        #
        loss = -tf.reduce_sum(groundtruth_lists * tf.log(prob+5e-38), 1)
        loss = tf.reduce_mean(loss)
        return loss

    def accuracy(self, prediction_dict, groundtruth_lists):
        """Calculate accuracy.

        Args:
            postprocessed_dict: A dictionary containing the postprocessed
                results
            groundtruth_lists: A dict of tensors holding groundtruth
                information, with one entry for each image in the batch.

        Returns:
            accuracy: The scalar accuracy.
        """
        pred_prob = prediction_dict['prob']
        rank = [i for i in range(70)]
        pred_age = tf.reduce_sum(pred_prob * rank, 1)
        # true_age = tf.reduce_sum(groundtruth_lists * rank, 1)
        # true_age = tf.cast(tf.argmax(groundtruth_lists,1), dtype=tf.float32)
        err = tf.abs(groundtruth_lists - pred_age)
        MAE = tf.reduce_mean(err)
        return MAE

    def variables_to_restore_and_train(self):

        exclude = ['MobilenetV1/Logits/Conv2d_1c_1x1']
        train_sc = ['MobilenetV1/Logits/Conv2d_1c_1x1']

        variables_to_restore = slim.get_variables_to_restore(exclude=exclude)
        variables_to_train = []
        for sc in train_sc:
            variables_to_train += slim.get_trainable_variables(sc)
        return variables_to_train, variables_to_restore

    def get_train_op(self, total_loss, variables_to_train, variables_to_restore, decay_steps, learning_rate, global_step):

        lr = tf.train.exponential_decay(learning_rate=learning_rate,
                                        global_step=global_step,
                                        decay_steps=decay_steps,
                                        decay_rate=0.8,
                                        staircase=True)

        update_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)
        with tf.control_dependencies(update_ops):
            opt1 = tf.train.AdamOptimizer(learning_rate=lr)
            opt2 = tf.train.AdamOptimizer(learning_rate=0.1 * lr)
            # opt2 = tf.train.GradientDescentOptimizer(0.01 * lr)
            grads = tf.gradients(total_loss, variables_to_train + variables_to_restore)
            grads1 = grads[:len(variables_to_train)]
            grads2 = grads[len(variables_to_train):]
            train_op1 = opt1.apply_gradients(zip(grads1, variables_to_train), global_step)
            train_op2 = opt2.apply_gradients(zip(grads2, variables_to_restore))
            train_op = tf.group(train_op1, train_op2)




        return train_op, lr