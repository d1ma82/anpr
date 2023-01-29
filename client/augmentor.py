import Augmentor
import os

def run(number_samples, dir):

    p = Augmentor.Pipeline(dir, "./")
    p.random_distortion(probability=0.4, grid_width=4, grid_height=4, magnitude=1)
    p.shear(probability=0.5, max_shear_left=5, max_shear_right=5)
    p.skew_tilt(probability=0.8, magnitude=0.1)
    p.rotate(probability=0.7, max_left_rotation=5, max_right_rotation=5)
    p.sample(number_samples)

def augment(number_samples, dir):
    for entry in os.scandir(dir):
        if (not entry.is_file()):
            augment(number_samples, entry)
        else:
            run(number_samples, dir.path) 


augment(50, "./data/chars/test")