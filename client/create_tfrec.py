import tensorflow as tf
import numpy as np
import os
import cv2

def _bytes_feature(value):
    return tf.train.Feature(bytes_list=tf.train.BytesList(value=[value]))

def _int64_feature(value):
    return tf.train.Feature(int64_list=tf.train.Int64List(value=[value]))

def create_tfrecord(folder, file):
    print ("Folder: "+ folder, "Output: ", file)

    writer = tf.io.TFRecordWriter(file)
    
    label = 0
    for dirname, dirnames, filenames in os.walk(folder):

        dirnames.sort()
        for subdirname in dirnames:

            subject_path = os.path.join(dirname, subdirname)
            print (subject_path, "with label", label)
                
            for filename in os.listdir(subject_path):

                abs_path    = "%s/%s" % (subject_path, filename)
                print (abs_path)
                img_file    = open(abs_path, "rb")
                img_file.seek(0)
                img_data    = np.asarray(bytearray(img_file.read()), dtype=np.uint8)
                img         = cv2.imdecode(img_data, cv2.IMREAD_UNCHANGED)      # use in memory because opencv do not understand unicode directories
                img_file.close()
                img_raw     = img.tobytes()
                example     = tf.train.Example(features=tf.train.Features(feature={
		                        'label': _int64_feature(label),
		                        'data' : _bytes_feature(img_raw)
		        }))
                writer.write(example.SerializeToString())
            label = label + 1

    writer.close()

create_tfrecord("./data/chars/train", "./data/chars/train/train.tfrecords")