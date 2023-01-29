import tensorflow as tf


logits = tf.constant([[2., -5., .5, -.1],
                      [0., 0., 1.9, 1.4],
                      [-100., 100., -100., -100.]])
labels = tf.constant([0, 3, 0])
print(tf.nn.sparse_softmax_cross_entropy_with_logits(
    labels=labels, logits=logits))