#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

// OpenCV
#include "opencv2/opencv.hpp"
/*#include "opencv/cv.h"
#include "opencv/cxcore.h"
#include "opencv/highgui.h"
#include "opencv/cvaux.h"
#include "opencv2/videoio.hpp"
*/

#ifdef USE_RASPICAM
#include <raspicam/raspicam_cv.h>
#endif

#include "client.h"
#include "base64.h"

using namespace cv;
using namespace std;

// number of training images
#define NB_TRAINING_IMGS 2
// dimention of the image vector (computed later)
#define WIDTH 480
#define HEIGHT 320

static int _imgDim = 0;
static bool _showUI = true;
static CvHaarClassifierCascade *_p_cascade = NULL;
static CvMemStorage *_p_storage = NULL;
static IplImage _currFrame;
static  Mat _M_trainingVect;
static  Mat M_trainingMat;
#ifdef USE_RASPICAM
static raspicam::RaspiCam_Cv _capture;
#else
static VideoCapture _capture(0);
#endif

// counters used for frame capture
static int _searchSmileDur = 0;
static  int _happyCount = 0;
static  int _sadCount = 0;
std::vector<char> GetImageData(IplImage *aimg);
void captureFrame(void);
IplImage* getGreySmileImg(IplImage* arg_p_img);
void loadTrainingImages(void);
void handleKeyPress(int arg_keyPress);
static void clean();

std::vector<char> GetImageData(IplImage *aimg){
  std::vector<char> ret;
  for (int i = 0; i < aimg->imageSize; i++){
    ret.push_back(aimg->imageData[i]);
  }
  return ret;
}

void captureFrame(void)
{
  Mat M_imgTmp;

  _capture.grab();
  _capture.retrieve(M_imgTmp);//,raspicam::RASPICAM_FORMAT_RGB);

  // Get one frame
  IplImage imgTmp = M_imgTmp;
  //clone image - _currFrame = M_imgTmp corrupts image   
  _currFrame = *(cvCloneImage(&imgTmp));
}

/** returned image must be released with cvReleaseImage */
IplImage* getGreySmileImg(IplImage* arg_p_img)
{
  IplImage* p_greySmile = NULL;

  // find the face
  CvSeq *rects = cvHaarDetectObjects(arg_p_img, _p_cascade, _p_storage, 1.3, 4, CV_HAAR_DO_CANNY_PRUNING, cvSize(50, 50));
  if(rects->total == 0) //no face detected
  {
    printf("could not find any face in image\n");
    if(_showUI)
    {
      cvShowImage("result", arg_p_img);
    }
  }
  else //face detected
  {
    CvRect *roi = 0;
    for(int i = 0; i < rects->total; i++)
    {
      CvRect *r = (CvRect*) cvGetSeqElem(rects, i);

      // biggest rect
      if(roi == 0 || (roi->width * roi->height < r->width * r->height)) roi = r;
    
      if(_showUI)
      {
        cvRectangle(arg_p_img, 
            cvPoint(r->x, r->y), 
            cvPoint(r->x + r->width, r->y + r->height),
            CV_RGB(255,0,0), 1, 8, 0);
        cvShowImage("Face detection", arg_p_img);
      }
    }

    // copy the face in the biggest rect
    int suby = roi->height * 0.6;
    roi->height -= suby;
    roi->y += suby;
    int subx = (roi->width - roi->height) / 2 * 0.7;
    roi->width -= subx * 2;
    roi->x += subx;
    cvSetImageROI(arg_p_img, *roi);
    //take smile in face
    IplImage *subimg = cvCreateImage(cvSize(100, 100 * 0.7), 8, 3);
    p_greySmile = cvCreateImage(cvGetSize(subimg), IPL_DEPTH_8U, 1);
    cvResize(arg_p_img, subimg);
    cvCvtColor(subimg, p_greySmile, CV_RGB2GRAY);
    cvReleaseImage(&subimg);
    cvEqualizeHist(p_greySmile, p_greySmile);
    cvResetImageROI(arg_p_img);
  }
  return p_greySmile; 
}

void loadTrainingImages(void)
{
  // read the images size
  CvSize sz = cvGetSize(cvLoadImage("data/happy00.png"));
  _imgDim = sz.width * sz.height; // compute the vector image length
  _M_trainingVect = Mat::zeros(_imgDim, 1, CV_32FC1);

  // read the training set images
  char file[64];
  M_trainingMat = Mat(_imgDim, NB_TRAINING_IMGS, CV_32FC1);
  for(int i = 0; i < NB_TRAINING_IMGS/2; i++)
  {
    sprintf(file, "data/happy%02d.png", i);
    CvMat* tmp = cvLoadImageM(file, CV_LOAD_IMAGE_GRAYSCALE);
    assert(tmp != NULL);
    Mat m =cvarrToMat(tmp);
    m = m.t();
    m = m.reshape(1, _imgDim);
    m.convertTo(m, CV_32FC1);
    m.copyTo(M_trainingMat.col(i));
    _M_trainingVect = _M_trainingVect + m;
  }
  for(int i = 0; i < NB_TRAINING_IMGS/2; i++)
  {
    sprintf(file, "data/sad%02d.png", i);
    CvMat* tmp = cvLoadImageM(file, CV_LOAD_IMAGE_GRAYSCALE);
    assert(tmp != NULL);
    Mat m =cvarrToMat(tmp);
    m = m.t();
    m = m.reshape(1, _imgDim);
    m.convertTo(m, CV_32FC1);
    m.copyTo(M_trainingMat.col(i + NB_TRAINING_IMGS/2));
    _M_trainingVect = _M_trainingVect + m;
  }
  printf("%d training images have been loaded\n", NB_TRAINING_IMGS);
}

void handleKeyPress(int arg_keyPress)
{
  if(arg_keyPress == 27) //Escape
  {
    clean();
    exit(0);
  }
  else if(arg_keyPress == 'h')
  { 
    char file[32];
    //Add an happy training image
    IplImage* greySmileImg = getGreySmileImg(&_currFrame);
    if(greySmileImg != NULL)
    {
      sprintf(file, "data/happy%02d.png", _happyCount);
      cvSaveImage(file, greySmileImg);
      _happyCount++;
      cout << "Une image contente a ete enregistrée" << endl;
      cvShowImage("Recorded image", greySmileImg);
      
    }
  }
  else if(arg_keyPress == 's')
  {
    char file[32];
    //Add a sad training image
    IplImage* greySmileImg = getGreySmileImg(&_currFrame);
    if(greySmileImg != NULL)
    {
      sprintf(file, "data/sad%02d.png", _sadCount);
      cvSaveImage(file, greySmileImg);
      _sadCount++;
      cout << "Une image triste a ete enregistree" << endl;
      cvShowImage("Recorded image", greySmileImg);
    }
  }
  else if(arg_keyPress = 'c')
  {
    _searchSmileDur = time(NULL) + 10;
  }
  else
  {
    printf("key %d not handled\n", arg_keyPress); 
  }
}

void clean(void)
{
  // cleanup
  _capture.release();
  if(_showUI){
    cvDestroyWindow("result");
    cvDestroyWindow("logo");
    cvDestroyWindow("face");
  }
  cvReleaseHaarClassifierCascade(&_p_cascade);
  cvReleaseMemStorage(&_p_storage);
}
int main (int argc, const char * argv[])
{

  // MQTT init 
  client* mqttHdl;
  mqttHdl = new client(NULL, "127.0.0.1", 1883);
  mqttHdl->loop_start();
  if(argc > 1 && strcmp(argv[1],"noi") == 0) {
    _showUI = false;
    cout << "mode sans UI" << endl;
  }
  mqttHdl->publish(NULL,"sourire",strlen("connected"),"connected");
  IplImage *img_happy,*img_sad;
  if(_showUI) {
    img_happy = cvLoadImage("data/happy.jpg");
    img_sad = cvLoadImage("data/sad.jpg");
    cvShowImage("logo", img_sad);
  }

  loadTrainingImages();

  // calculate eigenvectors
  Mat mean = _M_trainingVect / (float)(NB_TRAINING_IMGS);
  Mat A = Mat(_imgDim, NB_TRAINING_IMGS, CV_32FC1);
  //remove mean component (got from all training images) from training images
  for(int i = 0; i < NB_TRAINING_IMGS; i++) A.col(i) = M_trainingMat.col(i) - mean;
  Mat C = A.t() * A;
  Mat V, L;
  eigen(C, L, V);

  // compute projection matrix Ut
  Mat U = A * V;
  Mat Ut = U.t();

  // project the training set to the faces space
  Mat trainset = Ut * A;

  // prepare for face recognition
  _p_storage = cvCreateMemStorage(0);
  _p_cascade = (CvHaarClassifierCascade*)cvLoad("data/haarcascades_cuda/haarcascade_frontalface_default.xml");

  printf("starting cam\n");

#ifdef USE_RASPICAM
  if(!_capture.open()) 
#else
    if(!_capture.isOpened())
#endif 
    {
      printf("No webcam where found");
      return 1;
    }

  //forcer N/B sur l'image
  //capture.set(CV_CAP_PROP_FORMAT,CV_8UC1);
  _capture.set(CV_CAP_PROP_FRAME_WIDTH,WIDTH);
  _capture.set(CV_CAP_PROP_FRAME_HEIGHT,HEIGHT);


  CvScalar red = CV_RGB(250,0,0);
  printf("subscribing\n");
  mqttHdl->subscribe(NULL,"/amiqual4home/machine_place/saeco_intelia/on_button_press/#");
  printf("start listening\n");
  while(1)
  {
    captureFrame();

    if(_showUI)
    {
      cvShowImage("Video stream", &_currFrame);
      getGreySmileImg(&_currFrame);
    }

    // read the keyboard & handle windowing events
    int c = cvWaitKey(50);

    if(c != -1)
      handleKeyPress(c & 0xFF);

    if(_searchSmileDur > time(NULL)) {
      IplImage* greySmileImg = getGreySmileImg(&_currFrame);
      if(greySmileImg == NULL) continue;

      // recognize mood
      double min = 1000000000000000.0;
      int mini;
      Mat subimgmat = cvarrToMat(greySmileImg);
      subimgmat = subimgmat.t();
      subimgmat = subimgmat.reshape(1, _imgDim);
      subimgmat.convertTo(subimgmat, CV_32FC1);
      Mat proj = Ut * subimgmat;
      // find the minimum distance vector
      for(int i = 0; i < NB_TRAINING_IMGS; i++)
      {
        double n = norm(proj - trainset.col(i));
        if(min > n)
        {
          min = n;
          mini = i < NB_TRAINING_IMGS/2?0:1;
        }
      }
      if(mini == 1 && _showUI)
      {
        cvShowImage("logo", img_sad);
        printf("sad\n");	
      }
      else {
        _searchSmileDur = 0;
        IplImage img_send(_currFrame);
        std::string to_encode = "";	
        for (int i = 0; i < _currFrame.imageSize; i++){
          to_encode.push_back(_currFrame.imageData[i]);
        }

        std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(to_encode.c_str()),to_encode.length());

        //publie le message sur le réseau
        mqttHdl->publish(NULL,"/amiqual4home/machine_place/saeco_intelia/sourire",strlen(encoded.c_str()),encoded.c_str());
        cout << "image published over mqtt" << endl;	

        if(_showUI){
          printf("show happy\n");
          cvShowImage("logo", img_happy);
          cvWaitKey(5);
        }
      }
      if(_showUI) {
        cvMoveWindow("logo", 670, 200);

        // show results
        cvShowImage("face", greySmileImg);
        cvMoveWindow("face", 670, 0);
        cvShowImage("result", &_currFrame);
        cvMoveWindow("result", 0, 0);
      }
    } else {
      if(_searchSmileDur != 0) {
        cout << "no smile found " <<endl;
        mqttHdl->publish(NULL,"/amiqual4home/machine_place/saeco_intelia/sourire",strlen("fail"),"fail");
        _searchSmileDur = 0;
      }

      usleep(10);
      if(mqttHdl->consume_message()){
        _searchSmileDur = time(NULL) + 10;
        printf("start looking for a smile\n");
      }
    }
  }

  clean();
  printf("Exiting smile detecion \n");
  return 0;
}
