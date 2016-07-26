#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

// OpenCV
#include "opencv/cv.h"
#include "opencv/cxcore.h"
#include "opencv/highgui.h"
#include "opencv/cvaux.h"

#include <raspicam/raspicam_cv.h>

#include "client.h"
#include "base64.h"
// number of training images
#define M 2
// dimention of the image vector (computed later)
#define WIDTH 480
#define HEIGHT 320

static int N = 0;
static bool interface = true;
using namespace cv;
using namespace std;

std::vector<char> GetImageData(IplImage *aimg){
    std::vector<char> ret;
    for (int i = 0; i < aimg->imageSize; i++){
        ret.push_back(aimg->imageData[i]);
    }
    return ret;
}

int main (int argc, const char * argv[])
{
    
    //system("pwd");
    //init de mosquitto
    client* mqttHdl;
    mqttHdl = new client(NULL, "127.0.0.1", 1883);
    mqttHdl->loop_start();
    if(argc > 1 && strcmp(argv[1],"noi") == 0) {
        interface = false;
        cout << "mode sans interface" << endl;
    }
    mqttHdl->publish(NULL,"sourire",strlen("connected"),"connected");
    IplImage *img_happy,*img_sad;
    if(interface) {
        img_happy = cvLoadImage("data/happy.jpg");
        img_sad = cvLoadImage("data/sad.jpg");
        cvShowImage("logo", img_sad);
    }
    // read the images size
    CvSize sz = cvGetSize(cvLoadImage("data/happy00.png"));
    N = sz.width * sz.height; // compute the vector image length

    // read the training set images
    char file[64];
    Mat I = Mat(N, M, CV_32FC1);
    Mat S = Mat::zeros(N, 1, CV_32FC1);
    for(int i = 0; i < M/2; i++)
    {
        sprintf(file, "data/happy%02d.png", i);
        CvMat* tmp = cvLoadImageM(file, CV_LOAD_IMAGE_GRAYSCALE);
        Mat m =cvarrToMat(tmp);
        m = m.t();
        m = m.reshape(1, N);
        m.convertTo(m, CV_32FC1);
        m.copyTo(I.col(i));
        S = S + m;
    }
    for(int i = 0; i < M/2; i++)
    {
        sprintf(file, "data/sad%02d.png", i);
        CvMat* tmp = cvLoadImageM(file, CV_LOAD_IMAGE_GRAYSCALE);
        Mat m =cvarrToMat(tmp);
        m = m.t();
        m = m.reshape(1, N);
        m.convertTo(m, CV_32FC1);
        m.copyTo(I.col(i + M/2));
        S = S + m;
    }

    // calculate eigenvectors
    Mat Mean = S / (float)M;
    Mat A = Mat(N, M, CV_32FC1);
    for(int i = 0; i < M; i++) A.col(i) = I.col(i) - Mean;
    Mat C = A.t() * A;
    Mat V, L;
    eigen(C, L, V);

    // compute projection matrix Ut
    Mat U = A * V;
    Mat Ut = U.t();

    // project the training set to the faces space
    Mat trainset = Ut * A;

    // prepare for face recognition
    CvMemStorage *storage = cvCreateMemStorage(0);
    CvHaarClassifierCascade *cascade = (CvHaarClassifierCascade*)cvLoad("data/haarcascades_cuda/haarcascade_frontalface_default.xml");

    raspicam::RaspiCam_Cv capture;
    printf("starting cam\n");
    //forcer N/B sur l'image
    //capture.set(CV_CAP_PROP_FORMAT,CV_8UC1);
    capture.set(CV_CAP_PROP_FRAME_WIDTH,WIDTH);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT,HEIGHT);
    if(!capture.open())
    {
        printf("No webcam where found");
        return 1;
    }


    // counters used for frame capture
    int happy_count = 0;
    int sad_count = 0;
    int search_smile = 0;
    Mat image_tmp;
    CvScalar red = CV_RGB(250,0,0);
    printf("subscribing\n");
    mqttHdl->subscribe(NULL,"/amiqual4home/machine_place/saeco_intelia/on_button_press/#");
    printf("start listening\n");
    while(1)
    {

        if(search_smile > time(NULL)) {
            capture.grab();
            capture.retrieve(image_tmp);//,raspicam::RASPICAM_FORMAT_RGB);
            // Get one frame
            IplImage copy = image_tmp;
            IplImage *img = &copy;

            // read the keyboard
            int c = cvWaitKey(50) & 255;
            if(c == 27) break; // quit the program when ESC is pressed

            // find the face
            CvSeq *rects = cvHaarDetectObjects(img, cascade, storage, 1.3, 4, CV_HAAR_DO_CANNY_PRUNING, cvSize(50, 50));
            if(rects->total == 0)
            {
                if(interface)
                    cvShowImage("result", img);
                continue;
            }
            CvRect *roi = 0;
            for(int i = 0; i < rects->total; i++)
            {
                CvRect *r = (CvRect*) cvGetSeqElem(rects, i);

                // biggest rect
                if(roi == 0 || (roi->width * roi->height < r->width * r->height)) roi = r;
            }

            // copy the face in the biggest rect
            int suby = roi->height * 0.6;
            roi->height -= suby;
            roi->y += suby;
            int subx = (roi->width - roi->height) / 2 * 0.7;
            roi->width -= subx * 2;
            roi->x += subx;
            cvSetImageROI(img, *roi);
            IplImage *subimg = cvCreateImage(cvSize(100, 100 * 0.7), 8, 3);
            IplImage *subimg_gray = cvCreateImage(cvGetSize(subimg), IPL_DEPTH_8U, 1);
            cvResize(img, subimg);
            cvCvtColor(subimg, subimg_gray, CV_RGB2GRAY);
            cvEqualizeHist(subimg_gray, subimg_gray);
            cvResetImageROI(img);
            switch(c) // capture a frame when H (happy) or S (sad) is pressed
            {
                char file[32];
                case 'h':
                sprintf(file, "data/happy%02d.png", happy_count);
                cvSaveImage(file, subimg_gray);
                happy_count++;
                cout << "Une image contente a ete enregistree" << endl;
                cvWaitKey(1000);
                break;
                case 's':
                sprintf(file, "data/sad%02d.png", sad_count);
                cvSaveImage(file, subimg_gray);
                sad_count++;
                cout << "Une image triste a ete enregistree" << endl;
                cvWaitKey(1000);
                break;
            }

            // recognize mood
            double min = 1000000000000000.0;
            int mini;
            Mat subimgmat = cvarrToMat(subimg_gray);
            subimgmat = subimgmat.t();
            subimgmat = subimgmat.reshape(1, N);
            subimgmat.convertTo(subimgmat, CV_32FC1);
            Mat proj = Ut * subimgmat;
            // find the minimum distance vector
            for(int i = 0; i < M; i++)
            {
                double n = norm(proj - trainset.col(i));
                if(min > n)
                {
                    min = n;
                    mini = i < M/2?0:1;
                }
            }
            if(mini == 1 && interface)
                cvShowImage("logo", img_sad);
            else {
                search_smile = 0;
                IplImage img_send(*img);
                std::string to_encode = "";	
                for (int i = 0; i < img->imageSize; i++){
                    to_encode.push_back(img->imageData[i]);
                }

                std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(to_encode.c_str()),to_encode.length());

                //publie le message sur le réseau
                mqttHdl->publish(NULL,"/amiqual4home/machine_place/saeco_intelia/sourire",strlen(encoded.c_str()),encoded.c_str());
                cout << "image published over mqtt" << endl;	

                if(interface)
                    cvShowImage("logo", img_happy);
            }
            if(interface) {
                cvMoveWindow("logo", 670, 200);

                // show results
                cvShowImage("face", subimg_gray);
                cvMoveWindow("face", 670, 0);
                cvShowImage("result", img);
                cvMoveWindow("result", 0, 0);
            }
        } else {
            if(search_smile != 0) {
                cout << "no smile found " <<endl;
                mqttHdl->publish(NULL,"/amiqual4home/machine_place/saeco_intelia/sourire",strlen("fail"),"fail");
                search_smile = 0;
            }

            usleep(10);
            if(mqttHdl->consume_message()){
                search_smile = time(NULL) + 10;
                printf("start looking for a smile\n");
            }
        }
    }

    // cleanup
    capture.release();
    if(interface){
        cvDestroyWindow("result");
        cvDestroyWindow("logo");
        cvDestroyWindow("face");
    }
    cvReleaseHaarClassifierCascade(&cascade);
    cvReleaseMemStorage(&storage);

    printf("done");
    return 0;
}
