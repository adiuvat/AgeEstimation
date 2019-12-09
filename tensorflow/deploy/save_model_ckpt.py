"""Train a CNN model to classifying 10 digits.

Example Usage:
---------------
python3 train.py \
    --images_path: Path to the training images (directory).
    --model_output_path: Path to model.ckpt.
"""

import cv2
import glob
import numpy as np
import os
import tensorflow as tf
import logging

import model2deploy

slim = tf.contrib.slim
flags = tf.app.flags

flags.DEFINE_string('train_path', '../datasets/Chalearn15/train', 'Path to training images.')
flags.DEFINE_string('valid_path', '../datasets/Chalearn15/valid', 'Path to validation images.')
flags.DEFINE_string('test_path', '../datasets/Chalearn15/test', 'Path to test images.')
flags.DEFINE_string('model_output_path', 'models/model', 'Path to model checkpoint.')
flags.DEFINE_string('model_path','../mobilenet_wiki/models/model.ckpt-49-8.41-114500','Path to a pretrained model.')
FLAGS = flags.FLAGS
# 超参数设置
EPOCH = 90          #遍历数据集次数
start_epoch = 0     # 定义已经遍历数据集的次数
BATCH_SIZE = 32     #批处理尺寸(batch_size)
LR = 0.01           #学习率

TRAINING_EXAMPLES_NUM = 2476

#开始微调
init_finetune = 0
#继续微调
continue_finetune = 0
#测试
to_test = 1
#部署
to_deploy = 0






def main(_):
    os.environ["CUDA_VISIBLE_DEVICES"] = "0"
    with tf.name_scope('Input'):
        inputs = tf.placeholder(tf.float32, shape=[1, 224, 224, 3], name='inputs')
        # labels = tf.placeholder(tf.float32, shape=[None, 101], name='labels')
        # is_training = tf.placeholder(dtype=tf.bool)

    cls_model = model2deploy.Model()
    preprocessed_inputs = cls_model.preprocess(inputs)
    prediction_dict = cls_model.predict(preprocessed_inputs, False)
    # loss = cls_model.loss(prediction_dict, labels)
    # mae = cls_model.accuracy(prediction_dict, labels)
    # total_loss = loss + mae
    # postprocessed_dict = cls_model.postprocess(prediction_dict)
    #
    # variables_to_train, variables_to_restore = cls_model.variables_to_restore_and_train()
    # print(variables_to_train)
    # global_step = tf.train.get_or_create_global_step()
    #
    # num_batches_per_epoch = TRAINING_EXAMPLES_NUM / BATCH_SIZE
    # decay_steps = int(num_batches_per_epoch)
    # train_op = cls_model.get_train_op(total_loss, variables_to_train, variables_to_restore, decay_steps,
    #                                   LR, global_step)

    # 设置保存模型
    var_list = tf.trainable_variables()
    g_list = tf.global_variables()
    bn_moving_vars = [g for g in g_list if 'moving_mean' in g.name]
    bn_moving_vars += [g for g in g_list if 'moving_variance' in g.name]
    var_list += bn_moving_vars
    saver = tf.train.Saver(var_list=var_list, max_to_keep=100)
    init = tf.global_variables_initializer()

    with tf.Session() as sess:
        sess.run(init)
        saver.restore(sess, '../models/model.ckpt-65-2.98-82500')
        saver.save(sess, FLAGS.model_output_path + ".ckpt-deploy")





if __name__ == '__main__':
    tf.app.run()
