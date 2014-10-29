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

#define CLEAR(x) memset(&(x), 0, sizeof (x))

#define IMG_WIDTH 	320
#define IMG_HEIGHT 	240
	
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
    fprintf(stderr,"%s error %d, %s¥n", s, errno, strerror (errno));
    exit (EXIT_FAILURE);
}
	
static int xioctl(int fd, int request, void * arg)
{
    int r;
	
    do r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);
	    return r;
}

static void process_image(const void *p)	
{
    FILE *fp;
    fp=fopen("picture.yuv","wb");
    fwrite(p, sizeof(unsigned char), IMG_HEIGHT*IMG_WIDTH*2, fp);
    fclose(fp);
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
            int r;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            tv.tv_sec = 5;		// タイムアウト時間を設定
            tv.tv_usec = 0;
            r = select(fd+1, &fds, NULL, NULL, &tv);  // USBカメラからのイベント待ち

            if (r == -1) {		// エラー処理
                if (EINTR == errno)
                    continue;
                    errno_exit("select");
            }
            if (r == 0) {			// タイムアウト処理
                fprintf (stderr, "select timeout¥n");
                    exit (EXIT_FAILURE);
            }
            if (read_frame()){	// 読み込みOKになったらフレームを1枚読み込む
                printf(".");
                break;
            } else {
                printf("EAGAIN¥n");
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
        buf.index       = i;		// バッファの識別子
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
	

