#include <stdio.h>
#include <cv.h>
#include <highgui.h>

// 顔検出(静止画)
int main(int argc, char* argv[]) {
	// 顔検出対象の画像データ用
	IplImage* tarImg;

	// 検出対象の画像ファイルパス
	char tarFilePath[] = "output.jpg";

	// 画像データの読み込み
	tarImg = cvLoadImage(tarFilePath, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);

	// 正面顔検出器の読み込み
	CvHaarClassifierCascade* cvHCC = (CvHaarClassifierCascade*)cvLoad("haarcascade_frontalface_default.xml");

	// 検出に必要なメモリストレージを用意する
	CvMemStorage* cvMStr = cvCreateMemStorage(0);

	// 検出情報を受け取るためのシーケンスを用意する
	CvSeq* face;

	// 画像中から検出対象の情報を取得する
	face = cvHaarDetectObjects(tarImg, cvHCC, cvMStr);

	for (int i = 0; i < face->total; i++) {
		//検出情報から顔の位置情報を取得
		CvRect* faceRect = (CvRect*)cvGetSeqElem(face, i);

		// 取得した顔の位置情報に基づき、矩形描画を行う
		cvRectangle(tarImg,
			cvPoint(faceRect->x, faceRect->y),
			cvPoint(faceRect->x + faceRect->width, faceRect->y + faceRect->height),
			CV_RGB(255, 0 ,0),
			3, CV_AA);
	}

	// 顔位置に矩形描画を施した画像を表示
	cvNamedWindow("face_detect");
	cvShowImage("face_detect", tarImg);

	// キー入力待ち
	cvWaitKey(0);

	// ウィンドウの破棄
	cvDestroyWindow("face_detect");

	// 用意したメモリストレージを解放
	cvReleaseMemStorage(&cvMStr);

	// カスケード識別器の解放
	cvReleaseHaarClassifierCascade(&cvHCC);
	
	// イメージの解放
	cvReleaseImage(&tarImg);

	return 0;
}
