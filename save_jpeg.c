/*
jpeg保存サンプル
*/

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include "caputure_jpeg.c"

unsigned long Rcolor(double y,double u,double v);
unsigned long Gcolor(double y,double u,double v);
unsigned long Bcolor(double y,double u,double v);

int main (void)
{
// JPEGオブジェクト, エラーハンドラを確保
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
// 変数の宣言
	int	i, j;
// 出力ファイルのハンドラを確保、出力ファイル名を指定
	FILE	*fp;
	char 	*filename = "output.jpg";
// 画像のサイズ
	//int width = 256;
	//int height = 256;
    int width = 320;
    int height = 240;

    char *p;
    FILE *fd2;
    int rgb_count=0;
    double y1,u,y2,v;

    if((fd2=fopen("image_test.yuv","r"))==NULL);
    //if((fd2=fopen("picture.yuv","r"))==NULL);

    open_device();		//　ビデオバイスをオープン
    init_device();		//　ビデオデバイスを初期化
    start_capturing();		//　画像キャプチャ開始
    mainloop();			//　メインループ処理
    stop_capturing();		//　画像キャプチャ停止
    uninit_device();		//　初期化前の状態に戻す
    close_device();		//　ビデオデバイスをクローズ

// エラーハンドラにデフォルト値を設定
	cinfo.err = jpeg_std_error(&jerr);

// JPEGオブジェクトを初期化
	jpeg_create_compress(&cinfo);

// 出力ファイルをオープン
	if ((fp = fopen(filename, "wb"))== NULL) {
		fprintf(stderr, "cannot open %s¥n", filename);
		exit(EXIT_FAILURE);
	}
// 出力ファイルを指定
	jpeg_stdio_dest(&cinfo, fp);

// 画像のパラメータを設定
	cinfo.image_width = width;		// 画像の幅
	cinfo.image_height = height;		// 画像の高さ
	cinfo.input_components = 3;		// 1pixelあたりのカラー要素数
	cinfo.in_color_space = JCS_RGB;		// 入力画像の形式
	jpeg_set_defaults(&cinfo);
	//jpeg_set_quality(&cinfo, 75, TRUE);	// 圧縮効率（0〜100）
    jpeg_set_quality(&cinfo, 100, TRUE);	// 圧縮効率（0〜100）

// 圧縮を開始
	jpeg_start_compress(&cinfo, TRUE);

// データ（RGB値）を生成
	JSAMPARRAY img = (JSAMPARRAY) malloc(sizeof(JSAMPROW) * height);
	for (i = 0; i < height; ++i) {
		img[i] = (JSAMPROW) malloc(sizeof(JSAMPLE) * 3 * width);
		for (j = 0; j < width; j++) {
          if(j%2==0){
              y1=fgetc(fd2);
              u=fgetc(fd2);
              y2=fgetc(fd2);
              v=fgetc(fd2);
  	          img[i][j*3 + 0] = Rcolor(y1, u, v);
	          img[i][j*3 + 1] = Gcolor(y1, u, v);
	          img[i][j*3 + 2] = Bcolor(y1, u, v);
            }
          else{
             img[i][j*3 + 0] = Rcolor(y2, u, v);
	          img[i][j*3 + 1] = Gcolor(y2, u, v);
	          img[i][j*3 + 2] = Bcolor(y2, u, v);
            }
		}
	}
// 1行でなく画像全体を出力
	jpeg_write_scanlines(&cinfo, img, height); 

// 圧縮を終了
	jpeg_finish_compress(&cinfo);

// JPEGオブジェクトを破棄
	jpeg_destroy_compress(&cinfo);

	for (i = 0; i < height; i++) {
		free(img[i]);
	}
	free(img);
	fclose(fp);
    fclose(fd2);
}

unsigned long Rcolor(double y,double u,double v)
{
    double output_R=0;
    output_R=1.164*(y-16)+1.569*(v-128);
    if(output_R>255) output_R=255;
    if(output_R<0) output_R=0;
    return output_R;
}
unsigned long Gcolor(double y,double u,double v)
{
    double output_G=0;
    output_G=1.164*(y-16)-0.391*(u-128)-0.813*(v-128);
    if(output_G>255) output_G=255;
    if(output_G<0) output_G=0;
    return output_G;
}
unsigned long Bcolor(double y,double u,double v)
{
    double output_B=0;
    output_B=1.164*(y-16)+2.018*(u-128);
    if(output_B>255) output_B=255;
    if(output_B<0) output_B=0;
    return output_B;
}
