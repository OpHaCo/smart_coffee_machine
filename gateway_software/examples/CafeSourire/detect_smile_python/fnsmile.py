#!/usr/bin/env python
# encoding: utf-8
'''
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
import cv2
import time
import datetime
import traceback 
import numpy

from optparse import OptionParser

__all__ = []
__version__ = 0.1
__date__ = '2015-02-23'
__updated__ = '2015-09-19'

DEBUG = 0
TESTRUN = 0
PROFILE = 0


class HaarObjectTracker():
    
    def __init__(self, cascadeFile, childTracker=None, color=(0,255,0), id="", minNeighbors=0, verbose=0):
        self.detector = cv2.CascadeClassifier(cascadeFile)
        self.childTracker = childTracker
        self.color = color
        self.id = id
        self.verbose = verbose
        self.minNeighbors = minNeighbors
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
            childDetections = [(frame, 0, 0, frame.shape[1], frame.shape[0])]

        return childDetections
    

    def detect(self, childDetections):
        
        retDetections = []
           
        for (pframe, px, py, pw, ph) in childDetections:
            
            ownDetections = self.detector.detectMultiScale(pframe, scaleFactor=1.1,
                                                     minNeighbors=self.minNeighbors,
                                                     minSize=(30, 30),
                                                     flags=cv2.CASCADE_SCALE_IMAGE | cv2.CASCADE_DO_CANNY_PRUNING)
            for(x, y, w, h) in ownDetections :    
               
                #add detected rectangle
                rframe = pframe[y:y+h, x:x+w]
                # translate coordinate to correspond the the initial frame
                retDetections.append((rframe, x+px, y+py, w, h))

        return retDetections


    def draw(self, detectedObjects, drawFrame):
        
        for (frame, x, y, w, h) in detectedObjects:
            cv2.rectangle(drawFrame, (x, y), (x+w, y+h), self.color, 2)
            
    
    def trace(self, nbrObjects):
        if self.id != "" and nbrObjects > 0 and self.verbose > 0:
            print("{time} {id}: {n} detection(s)".format(time=datetime.datetime.now(), id=self.id, n=nbrObjects))

        
class SmileTracker(HaarObjectTracker):
    
    def __init__(self, cascadeFile, childTracker=None, color=(0,0,255), id="", minNeighbors=0, verbose=0):
        HaarObjectTracker.__init__(self, cascadeFile, childTracker, color, id, minNeighbors, verbose)
        
        #minimum smile duration
        self.MIN_SMILE_DUR = 300
        #currentsmile duration in ms
        self.smileDuration = 0

    def detect(self, childDetections ):
        
        detectedSmiles = []
        rawDetectedSmiles = super().detect(childDetections)
        haarCascadeCrit = 0
        
        if len(childDetections) > 0 :
            faceSize = childDetections[0][3]*childDetections[0][4]
            #criterion over number of detected smiles : depends on face size    
            haarCascadeCrit = faceSize*0.00476

        if len(rawDetectedSmiles) > haarCascadeCrit and self.fps > 0 :
            self.smileDuration = self.smileDuration + 1/self.fps*1000
            
            if(self.smileDuration > self.MIN_SMILE_DUR) :
                print("smile detected during {0}ms".format(self.smileDuration))
                #mean for found smile               
                rects = numpy.delete(rawDetectedSmiles, 0, 1)
                rects = numpy.mean(rects, axis = 0)
                detectedSmiles = [(rawDetectedSmiles[0][0], int(rects[0]), int(rects[1]), int(rects[2]), int(rects[3]))]
        else:
            self.smileDuration = 0
        
        return detectedSmiles


class FaceTracker(HaarObjectTracker):
    
    def __init__(self, cascadeFile, childTracker=None, color=(0,0,255), id="", minNeighbors=0, verbose=0):
        HaarObjectTracker.__init__(self, cascadeFile, childTracker, color, id, minNeighbors, verbose)
    

    def detect(self, childDetections):
        detectedFaces = super().detect(childDetections)
        
        #only get biggest face
        maxFace = [] 
        for (frame, x, y, w, h) in detectedFaces :
            if len(maxFace) == 0 or w*h > maxFace[0][3]*maxFace[0][4] :
                maxFace = [(frame, x, y, w, h)] 

        return maxFace
        
    
class LowerFaceTracker(HaarObjectTracker):
    
    def __init__(self, noseCascadeFile, childTracker=None, color=(0,0,255), id=""):
        HaarObjectTracker.__init__(self, noseCascadeFile, childTracker, color, id)

    
    def detectAndDraw(self, frame, drawFrame):
        
        if self.childTracker is not None:
            
            childDetections = self.childTracker.detectAndDraw(frame, drawFrame)
        
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
    
    


def main(argv=None):
    '''Command line options.'''
    
    program_name = os.path.basename(sys.argv[0])
    program_version = "v0.1"
    program_build_date = "%s" % __updated__
 
    program_version_string = '%%prog %s (%s)*****' % (program_version, program_build_date)
    program_usage = "Usage: python %s -s smile_model [-f face_model [-n nose_model]] [-d video_device].\
    \nFor more information run python %s --help" % (program_name, program_name)
    
    program_longdesc = "Read a video stream from a capture device or of file and track smiles on each frame.\
                        The default behavior is to scan the whole frame in search of smiles.\
                        If a face model is supplied (option -f), then search a smile is searched on the detected face.\
                        If a nose model is supplied (option -n, requires -f), the a smile is searched within the lower\
                        part of the face, taking the nose as a split point."
                        
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
        
    
        (opts, args) = parser.parse_args(argv)
        
        if opts.smile_model is None:
            raise Exception(program_usage)
        
        if opts.nose_model is not None and opts.face_model is None:
            raise Exception("Error: a nose detector also requires a face model")
        
        
        if opts.face_model is not None:
            faceTracker = FaceTracker(cascadeFile=opts.face_model, id="face-tracker", minNeighbors=15)
            
            if opts.nose_model is not None:
                lowerFaceTracker = LowerFaceTracker(noseCascadeFile = opts.nose_model, childTracker = faceTracker, id="nose-tracker")
                smileTracker = SmileTracker(opts.smile_model, lowerFaceTracker, (255,0,00), "smile-tracker", minNeighbors=0, verbose=1)
            else:
                smileTracker = SmileTracker(opts.smile_model, faceTracker, (255,0,0), "smile-tracker", minNeighbors=0, verbose=1)
            
        else:
            smileTracker = SmileTracker(opts.smile_model, None, (255,0,0), "smile-tracker", minNeighbors=0, verbose=1)
            
        
        try:
            videoCapture = cv2.VideoCapture(int(opts.video_source))
            videoCapture.grab()
        except ValueError as e:
            videoCapture = cv2.VideoCapture(opts.video_source)
        
        capturedFrames = 0
        fps = 0
        #smile duration in ms
        smileDurationCrit = 100
        smileDuration = 0

        while True:
            
            if capturedFrames == 0 :
                start = time.time()
            
            ret, frame = videoCapture.read()
            capturedFrames = capturedFrames + 1

            #fps = videoCapture.get(cv2.CAP_PROP_FPS)
            #print("Frames per second using video.get(cv2.CAP_PROP_FPS) : {0}".format(fps))
                         
            if not ret:
                break
            
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            
            # Adaptative histogram equalization
            #clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8,8))
            #gray = clahe.apply(gray)

            smileTracker.detectAndDraw(gray, frame, fps)
            
            cv2.imshow('Video', frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

            if capturedFrames == 40 :
                fps = capturedFrames / (time.time() - start)
                capturedFrames = 0
            
        videoCapture.release()
        cv2.destroyAllWindows()        
            
    
    except KeyboardInterrupt as k:
        sys.stderr.write("program will exit\nBye!\n")
        return 0
       
    except Exception as e:
        sys.stderr.write(str(e) + "\n")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        print(exc_type, fname, exc_tb.tb_lineno)
        return 2


if __name__ == "__main__":
    if DEBUG:
        sys.argv.append("-h")
    if TESTRUN:
        import doctest
        doctest.testmod()
    if PROFILE:
        import cProfile
        import pstats
        profile_filename = '_profile.txt'
        cProfile.run('main()', profile_filename)
        statsfile = open("profile_stats.txt", "wb")
        p = pstats.Stats(profile_filename, stream=statsfile)
        stats = p.strip_dirs().sort_stats('cumulative')
        stats.print_stats()
        statsfile.close()
        sys.exit(0)
    sys.exit(main())
