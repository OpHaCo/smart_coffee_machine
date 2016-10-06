#!/usr/bin/env python
# encoding: utf-8

'''
 @file    detect_smile.py 
 @author  Rémi Pincent - INRIA
 @date    10/07/2016

 @brief Detect smile from video capture. Also detects smiling subject.
 Results send over MQTT. 
 Project : smart_coffee_machine
 Contact:  Rémi Pincent - remi.pincent@inria.fr
 
 Revision History:
 https://github.com/OpHaCo/smart_coffee_machine.git 
  
  LICENSE :
  smart_coffee_machine (c) by Rémi Pincent
  smart_coffee_machine is licensed under a
  Creative Commons Attribution-NonCommercial 3.0 Unported License.
 
  You should have received a copy of the license along with this
  work.  If not, see <http://creativecommons.org/licenses/by-nc/3.0/>.

This file has been modified from : 

track--smile -- reads video stream from a capture device an d tracks smile.


Copyright (C) 2015 Amine SEHILI <amine.sehili@gmail.com>

genRSS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


@author:     Amine SEHILI
            
@copyright:  2015 Amine SEHILI
            
@license:    GNU GPL v03

@contact:    amine.sehili@gmail.com
@deffield    updated: Sept 2015
'''

import sys
import os

#raspberry check
isArmHw = False
if os.uname()[4][:3] == "arm" :
    print("   Program {0} runs on arm hardware".format(sys.argv[0]))
    isArmHw = True
else :
    print("   Program {0} runs on non arm hardware".format(sys.argv[0]))

import cv2
import time
import datetime
import traceback 
import numpy
import paho.mqtt.client as mqtt
import math 

#We suppose arm target is raspberry
if isArmHw :
    from picamera.array import PiRGBArray
    from picamera import PiCamera

from optparse import OptionParser
import threading
from threading import Thread

__all__ = []
__version__ = 0.1
__date__ = '2015-02-23'
__updated__ = '2015-09-19'

DEBUG = 0
TESTRUN = 0
PROFILE = 1 

# constants
MQTT_BROKER_ADD = "192.168.132.100"
CAPTURE_RES     = (480, 368)

_frame = []
_frameCounter = 0
_trainingFacesFolder = None
_mqttClient = None
_faceTracker = None
_smileTracker = None
_lowerFaceTracker = None
_stopThreads = False
_frameLock = threading.Lock()

class HaarObjectTracker():
    
    def __init__(self, cascadeFile, childTracker=None, color=(0,255,0), id="", minNeighbors=0, minSize=(100, 100), verbose=0):
        self.detector = cv2.CascadeClassifier(cascadeFile)
        self.childTracker = childTracker
        self.color = color
        self.id = id
        self.verbose = verbose
        self.minNeighbors = minNeighbors
        self.minSize = minSize
        self.fps = 0 

    def detectAndDraw(self, frame, drawFrame, fps):
        
        self.fps = fps
        childDetections = self.detectChild(frame, drawFrame)
        detections = self.detect(childDetections)
        self.draw(detections, drawFrame)    
        self.trace(len(detections))
        return detections 


    def detectChild(self, frame, drawFrame):
        
        if self.childTracker is not None:
            childDetections = self.childTracker.detectAndDraw(frame, drawFrame, self.fps)
        
        else:
            childDetections = [(frame, 0, 0, frame.shape[1], frame.shape[0], None)]

        return childDetections
    

    def detect(self, childDetections):
        
        retDetections = []
        for (pframe, px, py, pw, ph, id) in childDetections:
            
            ownDetections = self.detector.detectMultiScale(pframe, scaleFactor=1.1,
                                                     minNeighbors=self.minNeighbors,
                                                     minSize=self.minSize,
                                                     flags=cv2.CASCADE_SCALE_IMAGE | cv2.CASCADE_DO_CANNY_PRUNING | cv2.CASCADE_FIND_BIGGEST_OBJECT)
            for(x, y, w, h) in ownDetections :    
               
                #add detected rectangle
                rframe = pframe[y:y+h, x:x+w]
                # translate coordinate to correspond the the initial frame
                retDetections.append((rframe, x+px, y+py, w, h, id))

        return retDetections


    def draw(self, detectedObjects, drawFrame):
        
        for (frame, x, y, w, h, id) in detectedObjects:
            cv2.rectangle(drawFrame, (x, y), (x+w, y+h), self.color, 2)
            
    
    def trace(self, nbrObjects):
        if self.id != "" and nbrObjects > 0 and self.verbose > 0:
            print("{time} {id}: {n} detection(s)".format(time=datetime.datetime.now(), id=self.id, n=nbrObjects))
        
class SmileTracker(HaarObjectTracker):
    
    def __init__(self, cascadeFile, childTracker=None, color=(0,0,255), id="", minNeighbors=0, minSize=(10,10), verbose=0, mqttClient=None):
        HaarObjectTracker.__init__(self, cascadeFile, childTracker, color, id, minNeighbors, minSize, verbose)
        
        #minimum smile duration
        self.MIN_SMILE_DUR = 500
        #currentsmile duration in ms
        self.smileDuration = 0
        self.smileOnGoing = False 
        self.mqttClient = mqttClient

    def detect(self, childDetections ):
        
        detectedSmiles = []
        rawDetectedSmiles = super().detect(childDetections)
        haarCascadeCrit = 0
       

        if len(childDetections) > 0 :
            faceSize = childDetections[0][3]*childDetections[0][4]
            #criterion over number of detected smiles : depends on face size    
            haarCascadeCrit = math.sqrt(faceSize)*1.66 - 82
            mess="face size = {}x{} - number of raw smile  / criterion : {} / {}".format(childDetections[0][3], childDetections[0][4], len(rawDetectedSmiles), haarCascadeCrit)
            if haarCascadeCrit <= 5 :
                mess += " criterion <= 20 - face too small to detect a smile"
            print(mess)   

        if haarCascadeCrit > 20 and len(rawDetectedSmiles) > haarCascadeCrit and self.fps > 0 :
            self.smileDuration = self.smileDuration + 1/self.fps*1000
            
            if(self.smileDuration > self.MIN_SMILE_DUR) :
                if(not self.smileOnGoing) :
                    self.smileOnGoing = True
                    print("smile detected")
                    if self.mqttClient is not None :
                        self.mqttClient.publish("/CafeSourire/smile", rawDetectedSmiles[0][5])
            
                #mean for found smile
                #remove first column (frame) 
                rects = numpy.delete(rawDetectedSmiles, 0, 1)
                #remove last column (id)
                rects = numpy.delete(rects, -1, 1)
                rects = numpy.mean(rects, axis = 0)
                detectedSmiles = [(rawDetectedSmiles[0][0], int(rects[0]), int(rects[1]), int(rects[2]), int(rects[3]), rawDetectedSmiles[0][5])]
        else:
            if self.smileOnGoing :
                print("smile duration was {}".format(self.smileDuration))
            self.smileOnGoing = False
            self.smileDuration = 0
        
        return detectedSmiles
    
    
class FaceTracker(HaarObjectTracker):
    
    def __init__(self, cascadeFile, childTracker=None, color=(0,0,255), id="", minNeighbors=0, minSize=(50,50), verbose=0, recModel="LBPH"):
        HaarObjectTracker.__init__(self, cascadeFile, childTracker, color, id, minNeighbors, minSize, verbose)
        self.captureFace = [False, ""]
        self.faceRecModel = None
        self.trainingImages = []
        self.labels = []
        self.subjectIds = {}        
        self.recModel=recModel
        
        if self.loadTrainingImgs(_trainingFolder) :
            initTime = time.time()
            self.train()
            print("Training with {} images took {}ms".format(len(self.trainingImages), (time.time() - initTime)*1000))
        
        else :
            print("No face detection available")

    def detect(self, childDetections):

        detectedFaces = super().detect(childDetections)
        
        #only get biggest face
        maxFace = [] 
        for (frame, x, y, w, h, id) in detectedFaces :
            if len(maxFace) == 0 or w*h > maxFace[0][3]*maxFace[0][4] :
                maxFace = [(frame, x, y, w, h)]
         
        if len(maxFace) != 0 :
            resizedFace = cv2.resize(frame, (150, 175), interpolation = cv2.INTER_AREA)

            #Append id to frame - id = name of detected face
            maxFace[0] = maxFace[0] + (self.recognizeFace(resizedFace),) 

            if self.captureFace[0] :
                faceFolder = _trainingFolder + '/' + self.captureFace[1] + '/'
                if not os.path.exists(faceFolder):
                    os.makedirs(faceFolder)
                nbImgs = len([name for name in os.listdir(faceFolder) if os.path.isfile(os.path.join(faceFolder, name))])
                ret = cv2.imwrite(faceFolder + str(nbImgs+1) + ".pgm", resizedFace)
                if ret :
                    print("Face of {0} has been captured - training set for {1} = {2}".format(self.captureFace[1], self.captureFace[1], nbImgs+1))
                self.captureFace[0] = False
                 
         
        return maxFace 
        
    def captureFaceAsync(self, faceName):
        self.captureFace = [True, faceName]

    def loadTrainingImgs(self, imageFolder):

        if(not os.path.isdir(imageFolder)) :
            raise Exception("Invalid training image folder given.")

        label = 0
        for dirname, dirnames, filenames in os.walk(imageFolder):
            for subdirname in dirnames:
                subject_path = os.path.join(dirname, subdirname)
                subjectId = subject_path.split("/")[-1]
                for filename in os.listdir(subject_path):
                    abs_path = "%s/%s" % (subject_path, filename)
                    self.trainingImages.append(cv2.imread(abs_path, 0))
                    self.labels.append(label)
                self.subjectIds[label] = subjectId
                label = label + 1	
        if(len(self.trainingImages) <= 1) :
            print("Not enough training images - at least 2 images must be given")              
            return False
        else :
            print("{} images have been read".format(len(self.trainingImages)))            
            return True

    def train(self):
        
        #Get the height from the first image. We'll need this
        # later in code to reshape the images to their original
        # size:
        height = len(self.trainingImages[0])
        print("training image height = {}".format(height))
        self.trainingImages.pop();
        self.labels.pop();

        print("Face rec model is {}".format(self.recModel))
        if self.recModel == "PCA" :
            # The following lines create an Eigenfaces model for
            # face recognition and train it with the images and
            # labels read from training images.
            # This here is a full PCA, if you just want to keep
            # 10 principal components (read Eigenfaces), then call
            # the factory method like this:
            #
            #      cv::createEigenFaceRecognizer(10);
            #
            # If you want to create a FaceRecognizer with a
            # confidence threshold (e.g. 123.0), call it with:
            #
            #      cv::createEigenFaceRecognizer(10, 123.0);
            #
            # If you want to use _all_ Eigenfaces and have a threshold,
            # then call the method like this:
            #
            #      cv::createEigenFaceRecognizer(0, 123.0);
            #
            # Cannot set threshold : https://github.com/opencv/opencv_contrib/issues/512
            self.faceRecModel = cv2.face.createEigenFaceRecognizer(threshold=3000)
        
        else : #LBPH    
	    # The following lines create an LBPH model for
	    # face recognition and train it with the images and
	    # labels read from the given CSV file.
	    #
	    # The LBPHFaceRecognizer uses Extended Local Binary Patterns
	    # (it's probably configurable with other operators at a later
	    # point), and has the following default values
	    #
	    #      radius = 1
	    #      neighbors = 8
	    #      grid_x = 8
	    #      grid_y = 8
	    #
	    # So if you want a LBPH FaceRecognizer using a radius of
	    # 2 and 16 neighbors, call the factory method with:
	    #
	    #      cv::createLBPHFaceRecognizer(2, 16);
	    #
	    # And if you want a threshold (e.g. 123.0) call it with its default values:
            # cv::createLBPHFaceRecognizer(1,8,8,8,123.0)
            self.faceRecModel = cv2.face.createLBPHFaceRecognizer(threshold=80)

        self.faceRecModel.train(numpy.asarray(self.trainingImages), numpy.asarray(self.labels))  

    def recognizeFace(self, face) : 
        if self.faceRecModel is not None :
            #try to detect face from eigen vectors
            # confidence no more accessible when using python 3.5 and and opencv 3.1.0 
            # https://github.com/opencv/opencv_contrib/issues/512
            # Solution (archlinux) : install opencv-git and modify PKGBUILD to add
            # opencv_contrib (add source and compilation flag)
            foundLabel, confidence = self.faceRecModel.predict(face)
            
            if foundLabel == -1 :
                print("Unable to recognize any face")
                return "NA"
            else :
                print("Detected face label = {} , confidence = {} - subject id = {}".format(foundLabel, confidence, self.subjectIds[foundLabel]))
                #cv2.imshow("frame resized", face)
                #cv2.imshow("found image", self.trainingImages[self.labels.index(foundLabel)]) 
            
                return self.subjectIds[foundLabel]
        else :
            return "NA" 
    
class LowerFaceTracker(HaarObjectTracker):
    
    def __init__(self, noseCascadeFile, childTracker=None, color=(0,0,255), id="", minNeighbors=10, minSize=(40,40)):
        HaarObjectTracker.__init__(self, noseCascadeFile, childTracker, color, id, minNeighbors, minSize)

    
    def detectAndDraw(self, frame, drawFrame, fps):
        
        if self.childTracker is not None:
            
            childDetections = self.childTracker.detectAndDraw(frame, drawFrame, fps)
        
        else:
            childDetections = [(frame, 0, 0, frame.shape[1], frame.shape[0])]
         
        retDetections = []
           
        for (pframe, px, py, pw, ph) in childDetections:
            
            noses = self.detector.detectMultiScale(pframe, scaleFactor=1.1,
                                                     minNeighbors=self.minNeighbors,
                                                     minSize=(30, 30),
                                                     flags=cv2.CASCADE_SCALE_IMAGE)
            
            # select the lowest nose
            lowest_y = 999999
            for (nx, ny, nw, nh) in noses:
                if ny < lowest_y:
                    lowest_y = ny
                    lx = ny
                    lw = nw
                    lh = nh

            if lowest_y == 999999: # no nose detected
                continue
            
            
            # draw an ellipse around the lowest detected nose
            cv2.ellipse(drawFrame, (px+lx , py+lowest_y + int(lh/2)) , (lw//2,lh//2), 0, 0, 360, self.color, 2)
            
            # extract the lowest part of the face starting from the center of the nose
            center_y = lowest_y + int(float(lh) / 2)
            lowerFrame = pframe[center_y:,0:]
            
            # translate coordinates to correspond to the the initial frame
            retDetections.append((lowerFrame, px, py + center_y, lowerFrame.shape[1], lowerFrame.shape[0]))
            
            # lower face's rectangle
            cv2.rectangle(drawFrame, (px + 2, py + center_y), (px + pw - 2, py + ph - 2), (255, 255, 255), 2)
            
        self.trace(len(retDetections))
            
        return retDetections

def connectMQTT() :
    global _mqttClient 
    
    try :
        _mqttClient = mqtt.Client()
        _mqttClient.on_connect = onMQTTConnect
        _mqttClient.on_message = onMQTTMessage
        _mqttClient.connect_async(MQTT_BROKER_ADD, 1883, 60) 
        
        _mqttClient.loop_start()
    
    except Exception as e :
        print("Cannot connect MQTT server - " + str(e))
        _mqttClient.loop_stop()
        _mqttClient = None

def onMQTTConnect(client, userdata, flags, rc):
    print("Connected to MQTT broker with result code "+str(rc))
        
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("/smile_tracker/capture")

def onMQTTMessage(client, userdataMess, msg) :
    print("Client received message "  + msg.payload.decode("ascii") + " on topic " + msg.topic)
    
    if _faceTracker is not None :
        print("Capture a frame for " + msg.payload.decode("ascii"))
        _faceTracker.captureFaceAsync(msg.payload.decode("ascii"))

def handleFrame(frame, tracker, opts) :

    if handleFrame.capturedFrames == 0 :
        handleFrame.start = time.time()
    
    handleFrame.capturedFrames += 1

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    
    # Adaptative histogram equalization
    clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8,8))
    gray = clahe.apply(gray)
    
    if handleFrame.fps != 0 :
        tracker.detectAndDraw(gray, gray, handleFrame.fps)

    #keyboard actions on Video window
    if opts.video_stream :
        cv2.imshow('Video', gray)
        key = cv2.waitKey(1)
        
        if key & 0xFF == ord('q'):
            return False 
        elif key & 0xFF == ord('c'):
            print("Capture face...")
            _faceTracker.captureFaceAsync("Pedro")

    if handleFrame.capturedFrames == 40 :
        handleFrame.fps = handleFrame.capturedFrames / (time.time() - handleFrame.start)
        handleFrame.capturedFrames = 0
        print("(processed) fps = {}".format(handleFrame.fps))
        
    return True

#handleFrame() static variables
handleFrame.fps = 0
handleFrame.capturedFrames = 0
handleFrame.start = 0

def captureThread(opts):
    try :
        if isArmHw :
            rpiCapture(opts)

        else :
            capture(opts)
    
    except KeyboardInterrupt as k:
        print("Keyboard interrupt in capture thread")
    
    finally :
        print("Capture thread exit")


def capture(opts):
    global _frame
    global _frameCounter

    try :
        videoCapture = cv2.VideoCapture(int(opts.video_source))
        videoCapture.set(cv2.CAP_PROP_FRAME_HEIGHT, CAPTURE_RES[0]) 
        videoCapture.set(cv2.CAP_PROP_FRAME_WIDTH, CAPTURE_RES[1]) 
        # Do not work on official packages : http://stackoverflow.com/questions/7039575/how-to-set-camera-fps-in-opencv-cv-cap-prop-fps-is-a-fake
        #videoCapture.set(cv2.CAP_PROP_FPS, 10)
    
    except ValueError as e:
        videoCapture = cv2.VideoCapture(opts.video_source)
    
    while not _stopThreads :
        ret = videoCapture.grab()
        if not ret:
            time.sleep(0.01)
            break
        ret, frame = videoCapture.retrieve()    
        if not ret:
            time.sleep(0.01)
            break
        with _frameLock :
            #copy frame handled by video capturing structure, so it won't be modified by next captures
            _frame = numpy.copy(frame)

            if(_frameCounter == 0) :
                print("Captured image size = {}x{}".format(_frame.shape[1], _frame.shape[0]))
            _frameCounter += 1
            
    videoCapture.release()


def rpiCapture(opts):

    global _frame
    global _frameCounter

    # initialize the camera and grab a reference to the raw camera capture
    camera= PiCamera()
    camera.resolution = CAPTURE_RES 
    camera.framerate = 32
    rawCapture = PiRGBArray(camera, size=CAPTURE_RES)
    
    # allow the camera to warmup
    time.sleep(0.1)
     
    # capture frames from the camera
    for frame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True) :
	# grab the raw NumPy array representing the image, then initialize the timestamp
	# and occupied/unoccupied text
   
        with _frameLock :
            #copy frame handled by video capturing structure, so it won't be modified by next captures
            _frame = numpy.copy(frame.array)
            if(_frameCounter == 0) :
                print("Captured image size = {}x{}".format(_frame.shape[1], _frame.shape[0]))
            _frameCounter += 1

	# clear the stream in preparation for the next frame
        rawCapture.truncate(0)    
        
        if _stopThreads :
            return


def main(argv=None):
    global _trainingFolder
    global _faceTracker
    global _smileTracker
    global _lowerFaceTracker
    global _stopThreads
    captureTh = None
    
    '''Command line options.'''
    
    program_name = os.path.basename(sys.argv[0])
    program_version = "v0.1"
    program_build_date = "%s" % __updated__
 
    program_version_string = '%%prog %s (%s)*****' % (program_version, program_build_date)
    program_usage = "Usage: python %s -s smile_model [-f face_model [-n nose_model]] [-d video_device] [-stream].\
    \nFor more information run python %s --help" % (program_name, program_name)
    
    program_longdesc = "Read a video stream from a capture device or of file and track smiles on each frame.\
                        The default behavior is to scan the whole frame in search of smiles.\
                        If a face model is supplied (option -f), then search a smile is searched on the detected face.\
                        If a nose model is supplied (option -n, requires -f), the a smile is searched within the lower\
                        part of the face, taking the nose as a split point.\
                        If -a option given, capture stream and detected objects are displayed.\
                        If -t option given, then training faces are loaded from given folder.\
                        "
                        
    program_license = "Copyright 2015 Amine Sehili (<amine.sehili@gmail.com>).  Licensed under the GNU CPL v03"
 
    if argv is None:
        argv = sys.argv[1:]
    try:
        # setup option parser
        parser = OptionParser(version=program_version_string, epilog=program_longdesc, description=program_longdesc)
        parser.add_option("-f", "--face-model", dest="face_model", default=None, help="Cascade model for face detection (optional)", metavar="FILE")
        parser.add_option("-n", "--nose-model", dest="nose_model", default=None, help="Cascade model for nose detection (optional, requires -f)", metavar="FILE")
        parser.add_option("-s", "--smile-model", dest="smile_model", default=None, help="Cascade model for smile detection (mandatory)", metavar="FILE")
        parser.add_option("-v", "--video-source", dest="video_source", default="0", help="If an integer, try reading from the webcam, else open video file", metavar="INT/FILE")
        parser.add_option("-a", "--video-stream", action="store_true", dest="video_stream", default=False, help="Show video stream with detected objects (optional)")
        parser.add_option("-t", "--training-set", dest="training_set_folder", default=None, help="Face training set folder(mandatory)")
        parser.add_option("-m", "--rec-model", dest="rec_model", default="LBPH", help="Face recognizing model : PCA or LBPH")
        (opts, args) = parser.parse_args(argv)
        
        if opts.training_set_folder is None :
             raise Exception(program_usage)
         
        if not os.path.isdir(opts.training_set_folder) :
             raise Exception("given training folder do not exist")
       
        if opts.rec_model != "LBPH" and opts.rec_model != "PCA" :
            opts.rec_model = "LBPH"
         
        #remove trailing "/"
        if opts.training_set_folder[-1] == "/" :
            _trainingFolder = opts.training_set_folder[:-1]
        else :
            _trainingFolder = opts.training_set_folder 
            
        if opts.smile_model is None:
            raise Exception(program_usage)
        
        if opts.nose_model is not None and opts.face_model is None:
            raise Exception("Error: a nose detector also requires a face model")
        
        connectMQTT()        
        
        if opts.face_model is not None:
            #minSize parameter increase => min subject distance with camera needed to detect smile increase
            _faceTracker = FaceTracker(cascadeFile=opts.face_model, id="face-tracker", minNeighbors=15, minSize=(80,80), recModel=opts.rec_model)
            
            if opts.nose_model is not None:
                _lowerFaceTracker = LowerFaceTracker(noseCascadeFile = opts.nose_model, childTracker = _faceTracker, id="nose-tracker", minNeighbors = 10)
                _smileTracker = SmileTracker(opts.smile_model, _lowerFaceTracker, (255,0,00), "smile-tracker", minNeighbors=0, minSize=(10,10), verbose=1, mqttClient=_mqttClient)
            else:
                _smileTracker = SmileTracker(opts.smile_model, _faceTracker, (255,0,0), "smile-tracker", minNeighbors=0, minSize=(10,10), verbose=1, mqttClient=_mqttClient)
            
        else:
            _smileTracker = SmileTracker(opts.smile_model, None, (255,0,0), "smile-tracker", minNeighbors=0, minSize=(10,10), verbose=1, mqttClient=_mqttClient)

        # perform capture in a separate thread
        captureTh = Thread(target=captureThread, args=(opts,)) 
        captureTh.daemon = True 
        captureTh.start()
        
        currFrameIndex = 0 
        while captureTh.isAlive :
            _frameLock.acquire()
            if _frameCounter != currFrameIndex :
                #do a copy of frame because capture thread can write to _frame after lock is released.
                # => double buffering 
                #do a copy of _frameCounter as it can be modified after lock is released
                frameCounter = _frameCounter
                frame = numpy.copy(_frame)
                _frameLock.release()

                print("Handle frame {0}".format(_frameCounter))
                if not handleFrame(frame, _smileTracker, opts):
                    break
                currFrameIndex = frameCounter
            else:
                _frameLock.release()
                time.sleep(0.02)
    
    except KeyboardInterrupt as k:
        sys.stderr.write("program will exit\nBye!\n")
        return 0
       
    except Exception as e:
        sys.stderr.write("Exception in main thread : " + str(e) + "\n")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        print(exc_type, fname, exc_tb.tb_lineno)
        return 2

    finally :
        _stopThreads = True
        if captureTh is not None :
            while captureTh.isAlive() :
                time.sleep(0.01)
        
        cv2.destroyAllWindows()        

if __name__ == "__main__":
    if DEBUG:
        sys.argv.append("-h")
    if TESTRUN:
        import doctest
        doctest.testmod()
    if PROFILE:
        import pstats
        #Use yappi in multithreaded context
        import yappi
        
        yappi.start()
        exit_code = main()
        yappi.stop()
        stats = yappi.get_func_stats()
        tstats = yappi.get_thread_stats()
        STAT_FILE = 'profile_stats{}' 
        stats.save(STAT_FILE.format('.pstat'), type='pstat')
        
        with open(STAT_FILE.format('.txt'), 'w') as fh :
            # pstat format better than yappi format
            ps = pstats.Stats(STAT_FILE.format('.pstat'), stream = fh)
            ps.sort_stats('time')
            ps.print_stats()
            ps.sort_stats('cumulative')
            ps.print_stats()
            #append thread stats
            tstats.print_all(out = fh)
            #add function stat with yappi format
            stats.print_all(out=fh)

        sys.exit(exit_code)
    
    sys.exit(main())
