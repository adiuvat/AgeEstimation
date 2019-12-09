"""Train a CNN model to classifying 10 digits.

Example Usage:
---------------
python3 train.py \
    --images_path: Path to the training images (directory).
    --model_output_path: Path to model.ckpt.
"""
import time

import cv2
import glob
import numpy as np
import os
import tensorflow as tf
import logging

import model

slim = tf.contrib.slim
flags = tf.app.flags

flags.DEFINE_string('train_path', '../datasets/megaage_asian/train', 'Path to training images.')
flags.DEFINE_string('valid_path', '../datasets/megaage_asian/test', 'Path to validation images.')
# flags.DEFINE_string('test_path', '../datasets/Chalearn15/mytest', 'Path to test images.')
flags.DEFINE_string('model_output_path', 'models/model', 'Path to model checkpoint.')
flags.DEFINE_string('model_path','../mobilenet_wiki/models/model.ckpt-49-8.41-114500','Path to a pretrained model.')
FLAGS = flags.FLAGS
# 超参数设置
EPOCH = 90          #遍历数据集次数
start_epoch = 0     # 定义已经遍历数据集的次数
BATCH_SIZE = 32     #批处理尺寸(batch_size)
LR = 0.01           #学习率

TRAINING_EXAMPLES_NUM = 40000

#开始微调
init_finetune = 1
#继续微调
continue_finetune = 0
#测试
to_test = 0
#部署
to_deploy = 0


def get_data(images_path):
    """Get the training images from images_path.

    Args:
        images_path: Path to trianing images.

    Returns:
        images: A list of images.
        lables: A list of integers representing the classes of images.

    Raises:
        ValueError: If images_path is not exist.
    """
    if not os.path.exists(images_path):
        raise ValueError('images_path is not exist.')

    images = []
    labels = []
    images_path = os.path.join(images_path, '*.jpg')
    count = 0
    for image_file in glob.glob(images_path):
        count += 1
        if count % 100 == 0:
            print('Load {} images.'.format(count))
        image = cv2.imread(image_file)
        image = cv2.resize(image, (224, 224))
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        # image_raw_data = tf.gfile.FastGFile(image_file, 'rb').read()
        # img_data = tf.image.decode_jpeg(image_raw_data)
        # img_data = tf.image.convert_image_dtype(img_data, dtype=tf.float32)

        label_file = image_file.replace('.jpg', '.txt')
        label = np.loadtxt(label_file, dtype=np.float)
        # label += 1e-10
        images.append(image)
        labels.append(label)
    print(count)
    images = np.array(images)
    labels = np.array(labels)
    return images, labels

def get_test_data(images_path):
    """Get the training images from images_path.

    Args:
        images_path: Path to trianing images.

    Returns:
        images: A list of images.
        lables: A list of integers representing the classes of images.

    Raises:
        ValueError: If images_path is not exist.
    """
    if not os.path.exists(images_path):
        raise ValueError('images_path is not exist.')

    images = []
    labels = []
    images_path = os.path.join(images_path, '*.jpg')
    count = 0
    for image_file in glob.glob(images_path):
        count += 1
        if count % 100 == 0:
            print('Load {} images.'.format(count))
        image = cv2.imread(image_file)
        image = cv2.resize(image, (224, 224))
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        images.append(image)
        label_file = image_file.replace('.jpg', '.txt')
        # label = np.loadtxt(label_file, dtype=np.float)
        label=23.0
        labels.append(label)

    images = np.array(images)
    labels = np.array(labels)
    return images, labels


def next_batch_set(images, labels, batch_size=128):
    """Generate a batch training data.

    Args:
        images: A 4-D array representing the training images.
        labels: A 1-D array representing the classes of images.
        batch_size: An integer.

    Return:
        batch_images: A batch of images.
        batch_labels: A batch of labels.
    """
    indices = np.random.choice(len(images), batch_size)
    batch_images = images[indices]
    batch_labels = labels[indices]
    return batch_images, batch_labels

def get_batches(images ,labels, batch_size):
    image_batches = [images[i:i + batch_size] for i in range(0, len(images), batch_size)]
    label_batches = [labels[i:i + batch_size] for i in range(0, len(labels), batch_size)]
    # image_batches = np.array(image_batches)
    # label_batches = np.array(label_batches)

    return image_batches, label_batches

def shuffle_trainset(images, labels):
    state = np.random.get_state()
    np.random.shuffle(images)
    np.random.set_state(state)
    np.random.shuffle(labels)
    return images, labels



def main(_):
    os.environ["CUDA_VISIBLE_DEVICES"] = "0"
    with tf.name_scope('Input'):
        inputs = tf.placeholder(tf.float32, shape=[None, 224, 224, 3], name='inputs')
        labels_70 = tf.placeholder(tf.float32, shape=[None, 70], name='labels_70')
        labels_1 = tf.placeholder(tf.float32, shape=[None], name='labels_1')
        is_training = tf.placeholder(dtype=tf.bool)

    cls_model = model.Model()
    preprocessed_inputs = cls_model.preprocess(inputs, is_training)
    prediction_dict = cls_model.predict(preprocessed_inputs, is_training)
    loss = cls_model.loss(prediction_dict, labels_70)
    mae = cls_model.accuracy(prediction_dict, labels_1)
    total_loss = loss + mae
    postprocessed_dict = cls_model.postprocess(prediction_dict)

    variables_to_train, variables_to_restore = cls_model.variables_to_restore_and_train()
    print(variables_to_train)
    global_step = tf.train.get_or_create_global_step()

    num_batches_per_epoch = TRAINING_EXAMPLES_NUM / BATCH_SIZE
    decay_steps = int(num_batches_per_epoch)
    train_op = cls_model.get_train_op(total_loss, variables_to_train, variables_to_restore, decay_steps,
                                      LR, global_step)

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
        if continue_finetune:
            MODEL_SAVE_PATH = './models/'
            ckpt = tf.train.get_checkpoint_state(MODEL_SAVE_PATH)
            if ckpt and ckpt.model_checkpoint_path:
                saver.restore(sess, ckpt.model_checkpoint_path)

        # if to_deploy:
        #     saver.save(sess, FLAGS.model_output_path + ".ckpt-deploy")
        #     return

        if init_finetune:
            # 建立一个从预训练模型checkpoint中读取上述列表中的相应变量的参数的函数
            init_fn = slim.assign_from_checkpoint_fn(FLAGS.model_path, variables_to_restore, ignore_missing_vars=True)
            # restore模型参数
            init_fn(sess)

        if not to_test:
            images_train, labels_train = get_data(FLAGS.train_path)
            images_valid, labels_valid = get_data(FLAGS.valid_path)
            for epoch in range(start_epoch, EPOCH):
                epoch_loss = 0
                epcch_mae = 0
                images_train, labels_train = shuffle_trainset(images_train, labels_train)
                train_image_batches, train_label_batches = get_batches(images_train, labels_train, BATCH_SIZE)

                for i in range(len(train_image_batches)):
                    train_dict = {inputs: train_image_batches[i], labels_70: train_label_batches[i][:, 1:],
                                  labels_1: train_label_batches[i][:, 0], is_training: True}
                    # image_batch, label_batch = next_batch_set(images_train, labels_train, BATCH_SIZE)
                    # train_dict = {inputs: image_batch, labels: label_batch, is_training: True}
                    _, loss_, mae_ = sess.run([train_op, total_loss, mae], feed_dict=train_dict)
                    epoch_loss += loss_
                    epcch_mae += mae_
                    print('[epoch:%d, iter:%d] Loss: %.03f | Mae: %.03f '
                          % (epoch, (i + 1 + epoch * len(train_image_batches)), epoch_loss / (i + 1), epcch_mae / (i + 1)))


                print('run validation:')
                valid_image_batches, valid_label_batches = get_batches(images_valid, labels_valid, 100)
                valid_ae = 0
                valid_loss = 0
                for i in range(len(valid_image_batches)):
                    valid_dict = {inputs: valid_image_batches[i], labels_70: valid_label_batches[i][:, 1:],
                                  labels_1: valid_label_batches[i][:, 0], is_training: False}
                    loss_v, mae_v = sess.run([total_loss, mae], feed_dict=valid_dict)
                    valid_ae += mae_v * len(valid_image_batches[i])
                    valid_loss += loss_v * len(valid_image_batches[i])
                valid_text = 'Validation Loss: {} Mae: {}'.format(valid_loss/len(images_valid), valid_ae/len(images_valid))
                print(valid_text)
                saver.save(sess, FLAGS.model_output_path + ".ckpt-{}-{:.2f}".format(epoch, valid_ae / len(images_valid)),global_step)

        # if to_test:
        #     saver.restore(sess, './models/model.ckpt-10-3.40-858')
        #     images_test, labels_test = get_test_data(FLAGS.test_path)
        #     total_ae = 0
        #     for i in range(len(images_test)):
        #         test_dict = {inputs: [images_test[i]], is_training: False}
        #         postprocessed_dict_ = sess.run(postprocessed_dict, feed_dict= test_dict)
        #         age = postprocessed_dict_['age'][0]
        #         ae = abs(age-labels_test[i])
        #         total_ae += ae
        #         # rank = [i for i in range(101)]
        #         # age = (prob*rank).sum()
        #         print('{} pred: {} true: {} ae: {}'.format(i, age, labels_test[i], ae))
        #     print('MAE: {}'.format(total_ae/len(images_test)))


if __name__ == '__main__':
    tf.app.run()