#include <stdio.h>
#include <cv.h>
#include <highgui.h>

int main(int argc, char* argv[])
{

	IplImage *image = 0;
	CvCapture *capture = 0;

	capture = cvCaptureFromCAM(0);

	cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_WIDTH,640);
	cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_HEIGHT,480);
	
	if(capture){ 
		cvNamedWindow("Capture Image",1);

		while( 1 ){
			if( !(image = cvQueryFrame(capture)) )
			    break;
			
			cvShowImage("Capture Image", image);

			if( cvWaitKey(10) >= 0)
				break;
		}
		cvReleaseCapture(&capture);
		cvDestroyWindow("Capture Image");
	}

	return 0;
}
