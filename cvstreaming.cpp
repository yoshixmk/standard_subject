#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"

#define SCALE 1.3

// 物体の動きから移動量取得(動画)
int main(int argc, char* argv[]) {
    int fd;
	// ビデオキャプチャ構造体
	CvCapture *capture = 0;
	// フレーム単位データ
	IplImage *frame = 0;
	// フレーム単位データコピー用
	IplImage *frame_copy = 0;
	// 縦横サイズ
	double height = 240;
	double width = 320;
	// 入力キー受け付け用
	int c;
    fd = open("/sys/class/input/event0", O_RDONLY);  //ボタンが押されたら開放用 
    // 0番号のカメラに対するキャプチャ構造体を生成する
	capture = cvCreateCameraCapture (0);
	
	// キャプチャのサイズを設定する。ただし、この設定はキャプチャを行うカメラに依存するので注意る
	cvSetCaptureProperty (capture, CV_CAP_PROP_FRAME_WIDTH, width);
	cvSetCaptureProperty (capture, CV_CAP_PROP_FRAME_HEIGHT, height);
	cvNamedWindow ("capture_face_detect", CV_WINDOW_AUTOSIZE);
	
	// 停止キーが押されるまでカメラキャプチャを続ける
	while (1) {
		frame = cvQueryFrame (capture);

		// 顔位置に矩形描画を施した画像を表示
		cvShowImage ("capture_face_detect", frame);

		// 終了キー入力待ち(タイムアウト付き)
		c = cvWaitKey (2);//2が最速。デフォルト10
       //read(fd,&c,1);
		if (c == '0') {
			break;
		}
	}
	// キャプチャの解放
	cvReleaseCapture (&capture);
	
	// ウィンドウの破棄
	cvDestroyWindow("capture_face_detect");
	
	return 0;
}

