/*
 *  V4L2 video capture example
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <jpeglib.h>
// socket用
#include <netinet/ip.h>// signal用
#include <signal.h>
// getaddrinfo用
#include <netdb.h>

#define CLEAR(x) memset(&(x), 0, sizeof (x))

#define IMG_WIDTH 	320
#define IMG_HEIGHT 	240

int deal;
void process_image (char *p);
void hyozi(void);
int sendsample( int sd );
int ore_connect( char *host, int port );
	
struct buffer {
    void 	*start;
    size_t   	length;
};
	
static char *dev_name	= "/dev/video0";	// ビデオデバイス名
static int  fd	 	= -1;			// ビデオデバイスのハンドラ
struct buffer *buffers	= NULL;			// バッファへの先頭アドレス
static unsigned int n_buffers = 0;		// バッファ数
static char output_file[128];			// 出力ファイル
	
static void errno_exit(const char *s)
{
    fprintf(stderr,"%s error %d, %s\n", s, errno, strerror (errno));
    exit (EXIT_FAILURE);
}
	
static int xioctl(int fd, int request, void * arg)
{
    int r;
	
    do r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);
	    return r;
}
	
static int read_frame(void)
{
    struct v4l2_buffer buf;

    CLEAR (buf);
    buf.type 	 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory    = V4L2_MEMORY_MMAP;
		// outgoing queue から1フレーム分のバッファを取り出す
    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
       switch (errno) {	
            case EAGAIN:		// outgoing queueが空のとき
               return 0;
            default:
	        errno_exit("VIDIOC_DQBUF");
        }
    }
    assert(buf.index < n_buffers);
    process_image(buffers[buf.index].start); // フレームを処理
    xioctl(fd, VIDIOC_QBUF, &buf);	   // 使用したバッファをincoming queueに入れる
	
	hyozi();
    return 1;
}

static void mainloop(void)
{
    unsigned int count;
    count = 1;				// captureするフレーム数を指定
	
    while (count-- > 0) {
        for (;;) {
           fd_set fds;
            struct timeval tv;
            //int r;
              

            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            tv.tv_sec = 5;		// タイムアウト時間を設定
            tv.tv_usec = 0;
            deal = select(fd+1, &fds, NULL, NULL, &tv);  // USBカメラからのイベント待ち

            if (deal == -1) {		// エラー処理
				printf("deal==-1");
                if (EINTR == errno)
                    continue;
                    errno_exit("select");
            }
            if (deal == 0) {			// タイムアウト処理
				printf("deal==0");
                fprintf (stderr, "select timeout\n");
                    exit (EXIT_FAILURE);
            }
            if (read_frame()){	// 読み込みOKになったらフレームを1枚読み込む
                //printf(".");
                break;
            } else {
                printf("EAGAIN\n");
            }
              // EAGAIN : outgoing queueが空のとき繰り返す
        }
   }
}
	
static void stop_capturing(void)
{
    enum v4l2_buf_type type;
	
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);	// ストリーミングを停止
}
	
static void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;			// バッファの識別子
        xioctl(fd, VIDIOC_QBUF, &buf);	// incoming queueにバッファをエンキュー
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &type);	//　ストリーミングを開始
}

static void uninit_device(void)
{
    unsigned int i;

    for (i = 0; i < n_buffers; ++i)
        munmap(buffers[i].start, buffers[i].length);	// アンマップ
    free(buffers);					// メモリを開放
}

static void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR (req);
    req.count	= 4;				// バッファ数
    req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;	// バッファタイプ
    req.memory	= V4L2_MEMORY_MMAP;		// MMAPを使用
    xioctl(fd, VIDIOC_REQBUFS, &req);		// メモリマッピングの設定

    buffers = calloc(req.count, sizeof(*buffers));	//　動的にメモリを確保

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

       CLEAR(buf);
        buf.type	   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index	   = n_buffers;
        xioctl(fd, VIDIOC_QUERYBUF, &buf);

        buffers[n_buffers].length = buf.length;
		//　デバイスメモリとアプリケーションのメモリをマッピング（共有）
        buffers[n_buffers].start = mmap(NULL,	// start anywhere
		buf.length,			// バッファの長さ
                PROT_READ | PROT_WRITE,		// メモリ保護を指定
                MAP_SHARED,			// 共有を指定
                fd, buf.m.offset);		// ファイルハンドラ、オフセット
    }
}

static void init_device(void)
{
    struct v4l2_format fmt;

    CLEAR(fmt);
    fmt.type 		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width	= IMG_WIDTH; 	     // キャプチャ画像の解像度：幅
    fmt.fmt.pix.height	= IMG_HEIGHT;	     // キャプチャ画像の解像度：高さ
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;// ピクセルフォーマットの設定：YUYV
    fmt.fmt.pix.field  = V4L2_FIELD_INTERLACED; // 受信するビデオフォーマットの設定
    xioctl(fd, VIDIOC_S_FMT, &fmt);	     // 映像フォーマットの設定
	
    init_mmap();	// メモリマッピングの初期化
}
	
static void close_device(void)
{
    close(fd);
}
	
static void open_device(void)
{
    fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
}
	
int main(int argc, char *argv[])
{
    int sd;         // 通信ソケット
    int result;
    char *host;     // 接続先IPアドレス
    int port;       // 接続先ポート番号
    //strcpy(output_file, argv[1]);

    if( argc<3 ){
        printf("usage: %s <ipaddress> <port>\n",argv[0]);
        exit(0);
    }
    host = argv[1];
    port = atoi(argv[2]);
    
    signal(SIGPIPE, SIG_IGN);   // PIPE切断シグナルを無視する
    open_device();		//　ビデオバイスをオープン
    init_device();		//　ビデオデバイスを初期化
    start_capturing();		//　画像キャプチャ開始
	while(deal != 1){
    mainloop();			//　メインループ処理
	//hyozi();
	}
    stop_capturing();		//　画像キャプチャ停止
    uninit_device();		//　初期化前の状態に戻す
    close_device();		//　ビデオデバイスをクローズ
    return 0;
}

/*
jpeg保存サンプル
*/
void process_image (char *p)
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
	int width = 320;
	int height = 240;

// エラーハンドラにデフォルト値を設定
	cinfo.err = jpeg_std_error(&jerr);

// JPEGオブジェクトを初期化
	jpeg_create_compress(&cinfo);

// 出力ファイルをオープン
	if ((fp = fopen(filename, "wb"))== NULL) {
		fprintf(stderr, "cannot open %s\n", filename);
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
	jpeg_set_quality(&cinfo, 75, TRUE);	// 圧縮効率（0～100）

// 圧縮を開始
	jpeg_start_compress(&cinfo, TRUE);

// データ（RGB値）を生成

	int data[3],cnt;
	JSAMPARRAY img = (JSAMPARRAY) malloc(sizeof(JSAMPROW) * height);
	for (i = 0; i < height; i++) {
		img[i] = (JSAMPROW) malloc(sizeof(JSAMPLE) * 3 * width);
		for (j = 0; j < width/2; j++) {

			data[0] = 1.164*(*p-16)+1.569*((*(p+3))-128);
			data[1] = 1.164*(*p-16)-0.391*((*(p+1))-128)-0.813*((*(p+3))-128);
			data[2] = 1.164*(*p-16)+2.018*((*(p+1))-128);
			for(cnt=0 ; cnt<=2 ; cnt++){
			    if(data[cnt]>255){
				    data[cnt]=255;
				    }
			    if(data[cnt]<0){
				    data[cnt]=0;
				    }
			}

			img[i][j*6 + 0] = data[0];
			img[i][j*6 + 1] = data[1];
			img[i][j*6 + 2] = data[2];

			data[3] = 1.164*((*(p+2))-16)+1.569*((*(p+3))-128);
			data[4] = 1.164*((*(p+2))-16)-0.391*((*(p+1))-128)-0.813*((*(p+3))-128);
			data[5] = 1.164*((*(p+2))-16)+2.018*((*(p+1))-128);

			for(cnt=3 ; cnt<=5 ; cnt++){
			if(data[cnt]>255){
				data[cnt]=255;
				}
			if(data[cnt]<0){
				data[cnt]=0;
				}
			}

			img[i][j*6 + 3] = data[3];
			img[i][j*6 + 4] = data[4];
			img[i][j*6 + 5] = data[5];

			p = p + 4;

			/*
			R = 1.164*(y2-16)+1.569*(v-128);
			G = 1.164*(y2-16)-0.391*(u-128)-0.813*(v-128);
			B = 1.164*(y2-16)+2.018*(u-128);
			*/
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
	free( img );
	fclose( fp );
}





void hyozi(void){
	int fd;
	int j,i;
	fd = open("/dev/fb0",O_RDWR);
	struct fb_fix_screeninfo finfo;
	ioctl(fd,FBIOGET_FSCREENINFO,&finfo);
	struct fb_var_screeninfo vinfo;
	ioctl(fd,FBIOGET_VSCREENINFO,&vinfo);
	int width = vinfo.xres;
	int height = vinfo.yres;
	int depth = vinfo.bits_per_pixel;
	int size = width*height*depth / 8;
	int ox = vinfo.xoffset;
	int oy = vinfo.yoffset;
	int len = finfo.line_length;
	char *p;
	unsigned char *p1;
	FILE *fp;
	int r,g,b;
	unsigned long pct;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	fp = fopen("output.jpg","rb");
	jpeg_stdio_src(&cinfo,fp);

	jpeg_read_header(&cinfo,TRUE);

	jpeg_start_decompress(&cinfo);

	width = cinfo.output_width;
	height = cinfo.output_height;

	JSAMPARRAY img;

	img = (JSAMPARRAY)malloc(sizeof(JSAMPROW)*height);

	for(i=0;i<height;i++){
		img[i] = (JSAMPROW)calloc(sizeof(JSAMPLE),3*width);
	}

	while(cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo,img	+cinfo.output_scanline,cinfo.output_height-cinfo.output_scanline);
	}

	p = (char*)mmap(0,size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	for(j = 0; j<height; j++){
		p1=img[j];
		for(i=0;i<width; i++){
			pct=0;
			r=*p1++;
			g=*p1++;
			b=*p1++;
			r=r*32/256;
			g=g*64/256;
			b=b*32/256;
			pct=pct | r;
			pct=pct<<6;
			pct=pct | g;
			pct=pct<<5;
			pct=pct | b;
			//printf("%d,%d,%d\n",r,g,b);
			int index = (ox+i) * (depth/8)+(oy+j) * len;
			*((unsigned short int *)(p + index))=pct;
		}
	}
	munmap(p,size);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	close(fd);
	fclose(fp);
	for(i=0;i<height;i++)free(img[i]);
	free(img);
	//return 0;
}
int ore_connect( char *host, int port )
{
    int sd;                     // 待ち受けソケット
    struct addrinfo hints;      // bind用ヒント
    struct addrinfo *ai;        // bind用アドレス情報
    char portstr[16];
    int result;

    // 接続先の情報を設定
    memset(&hints,0,sizeof(hints));
    hints.ai_flags       = AI_ADDRCONFIG;
    hints.ai_family      = AF_UNSPEC;   // IPv6/IPv4 両方
    hints.ai_socktype    = SOCK_DGRAM;  // UDP です
    snprintf( portstr, sizeof(portstr), "%d", port );   // 待ち受けポート番号を文字列にする
    result = getaddrinfo( host, portstr, &hints, &ai );
    if( result!=0 ){
        fprintf( stderr,"getaddrinfo: %s\n", gai_strerror(result) );
        return -1;
    }

    // 接続用ソケット作成
    sd = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol );
    if( sd<0 ){
        perror("socket");
        freeaddrinfo(ai);
        return -1;
    }

    // 接続開始
    result = connect(sd, ai->ai_addr, ai->ai_addrlen);
    if( result<0 ){
        perror("connect");
        close(sd);
        freeaddrinfo(ai);
        return -1;
    }
    
    freeaddrinfo(ai);
    return sd;
}


/*! 
 @brief メインロジック部分
 
 20文字の文字列を送信し、受信した文字列を出力する
 @param[in] sd  ソケット
 @return    接続を維持するなら1を返す。正常に切断されたら0を、エラーなら-1を返す。
 @retval    1   接続を維持
 @retval    0   正常に切断検知
 @retval    -1  エラー
*/
int sendsample( int sd )
{
    int len;
    char buf[256];
    int result;
    
    
    // 書く
    strcpy( buf, "01234567890123456789" );
    len = strlen(buf);
    
    result = write( sd, buf, len );
    if( result==0 ){
        return 0;
    }else if( result<0 ){
        perror("write");
        return -1;
    }
    printf("%d:send:%.*s\n",sd,result,buf);

    // 読む
    result = read( sd, buf, sizeof(buf) );
    if( result==0 ){
        return 0;
    }else if( result<0 ){
        perror("read");
        return -1;
    }
    printf("%d:recv:%.*s\n",sd,len,buf);
    
    
    return 1;
}
