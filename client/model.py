import tensorflow as tf
import numpy as np
import os
import cv2
import matplotlib.pyplot as plt
from tensorflow.python.framework.convert_to_constants import convert_variables_to_constants_v2


num_classes     = 22
width           = 20
height          = 20
batch_size      = 100
epochs          = 10
pb_model_name   = 'anpr_10.pb'

def _parser(example_proto):
    features = {"label": tf.io.FixedLenFeature((), tf.int64, default_value=0),
                 "data": tf.io.FixedLenFeature((), tf.string, default_value="")
    }
    parsed_features = tf.io.parse_single_example(example_proto, features)
    image = tf.io.decode_raw(parsed_features['data'], tf.uint8)
    image = tf.cast(image, tf.float32)/255
    shape = tf.stack([width, height, 1])
    image = tf.reshape(image, shape)
    label = tf.one_hot(indices=parsed_features["label"], depth=num_classes, dtype=tf.int64)
    return image, label

dataset = tf.data.TFRecordDataset('./data/chars/train/train.tfrecords')
dataset = dataset.map(lambda x: _parser(x))
dataset = dataset.batch(batch_size)

model = tf.keras.Sequential([
    tf.keras.Input(shape=(width, height, 1)),
    tf.keras.layers.Conv2D(32, kernel_size=(5,5), activation='relu', padding='SAME'),
    tf.keras.layers.MaxPooling2D(pool_size=(2,2)),
    tf.keras.layers.Conv2D(64, kernel_size=(5,5), activation='relu', padding='SAME'),
    tf.keras.layers.MaxPooling2D(pool_size=(2,2)),
    tf.keras.layers.Flatten(),
    tf.keras.layers.Dropout(0.5),
    tf.keras.layers.Dense(num_classes, activation='softmax'),
])

#model.summary()

#model.compile(loss="categorical_crossentropy", optimizer='adam', metrics=['accuracy'])
#model.fit(dataset, epochs=epochs)

def get_opencv_dnn_prediction(opencv_net, preproc_img, labels):

    opencv_net.setInput(preproc_img)
    out = opencv_net.forward()
    print("OpenCV DNN prediction: \n")
    print("* shape: ", out.shape)

    class_id = np.argmax(out)
    confidence = out[0][class_id]
    print("* class ID: {}, label: {}".format(class_id, labels[0][class_id]))
    print("* confidence: {:.4f}\n".format(confidence))

def get_tf_model_proto(tf_model):

    pb_model_path = "models"
    os.makedirs(pb_model_path, exist_ok=True)

    tf_model_graph = tf.function(lambda x: tf_model(x))
    tf_model_graph = tf_model_graph.get_concrete_function(
        tf.TensorSpec(tf_model.inputs[0].shape, tf_model.inputs[0].dtype))

    # obtain frozen concrete function
    frozen_tf_func = convert_variables_to_constants_v2(tf_model_graph)
    frozen_tf_func.graph.as_graph_def()

    tf.io.write_graph(graph_or_graph_def    =   frozen_tf_func.graph,
                      logdir                =   pb_model_path,
                      name                  =   pb_model_name,
                      as_text               =   False)

    return os.path.join(pb_model_path, pb_model_name)

#print("Freeze model")
#pb_path     = get_tf_model_proto(model)     # Создадим граф сети для экспорта в OpenCV


# Эту часть нужно перенести в проект
opencv_net  = cv2.dnn.readNetFromTensorflow(os.path.join(f'./models/{pb_model_name}'))
print("OpenCV model was successfully read. Model layers: \n", opencv_net.getLayerNames())

testset = tf.data.TFRecordDataset('./data/chars/test/test.tfrecords')
testset = testset.map(lambda x: _parser(x))
testset = testset.batch(1)
testrecs = 1
i = 0
for test, label in testset:
    #print(test.numpy())
    blob=cv2.dnn.blobFromImage(image=test.numpy().reshape(width, height), size=(width, height))
    print("Input blob shape: {}\n".format(blob.shape))
    print("Label: shape", label)
    get_opencv_dnn_prediction(opencv_net, blob, label)
    i=i+1
    if i==testrecs: break

#score = model.evaluate(testset, verbose=0)
#print("Test loss:", score[0])
#print("Test accuracy:", score[1])
