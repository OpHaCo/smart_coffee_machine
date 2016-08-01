# Face Nose Smile detector
`fnsmile` is an OpenCV based smile detector which uses, besides a cascade smile model, a face model and a nose model to reduce false alarms as much as possible.

# Motivations
This code is supplied as a demo for this blog [article](https://aminesehili.wordpress.com/2015/09/20/smile-detection-with-opencv-the-nose-trick/). To summarize the main idea in a couple of words, here are the steps of the algorithm:
 
 - Detect all faces on a picture
 - Try to detect a nose on each face. Discard faces with no nose (reduces the face model's false alarms)
 - Use the lower part of the face, starting from the center of the nose, to detect a smile

# Usage
OpenCV's cascade models are usually located in `/usr/local/share/OpenCV/haarcascades/`. You will most certainly have to append the name of the model to this path.

## Use a smile detector alone
    python fnsmile.py -s haarcascade_smile.xml
 
 
## Use a smile detector + a face detector
    python fnsmile.py -s haarcascade_smile.xml -f haarcascade_frontalface_default.xml
   
## Use three detectors (smile, face and nose)
    python fnsmile.py -s haarcascade_smile.xml -f haarcascade_frontalface_default.xml -n haarcascade_mcs_nose.xml

## Use another ID for the webcam (default: 0)
    -v 1 

## Read data from a video file (default: from the webcam)
    -v filename


# Author
Amine Sehili (<amine.sehili@gmail.com>)

# License
This program is available under the GNU GENERAL PUBLIC LICENSE Version 3.
