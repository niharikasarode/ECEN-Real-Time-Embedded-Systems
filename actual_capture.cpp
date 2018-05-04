#include <opencv2/opencv.hpp>
//#include <opencv2/tracking/tracker.hpp>
#include <opencv2/core/ocl.hpp>
#include <unistd.h>

using namespace cv;
using namespace std;

int motion_detected = 0;

int main(int argc, char **argv) {
    // Mat is n channel array used to store grayscale images
    Mat frame, gray, frameDelta, thresh, firstFrame;
    vector<vector<Point> > cnts;//find contours in the image
    VideoCapture camera(0); //open camera

    //set the video size to 512x288 to process faster
    camera.set(3, 512);
    camera.set(4, 288);

    sleep(3);
    camera.read(frame);

    //convert to grayscale and set the first frame
    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);//convert to gray (i/p, o/p, int code)
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);//smooth out images

    while(camera.read(frame)) {

        //convert to grayscale
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);
        absdiff(firstFrame, gray, frameDelta);
        threshold(frameDelta, thresh, 65, 255, THRESH_BINARY);

        dilate(thresh, thresh, Mat(), Point(-1,-1), 2);
        findContours(thresh, cnts, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for(int i = 0; i< cnts.size(); i++) {
            if(contourArea(cnts[i]) < 500) {
                continue;
            }

            else
            {
                putText(frame, "Motion Detected", Point(10, 20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
                motion_detected = 1;
            }
        }

        imshow("Camera", frame);

        if(waitKey(1) == 27){
            //exit if ESC is pressed
            break;
        }

        if(motion_detected == 1)
        {
                motion_detected =0;
                break;
        }

    }

    return 0;
}