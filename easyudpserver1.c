/*! 
 @file
 @brief UDPサーバプログラム
 
 基本的なUDPサーバプログラムの流れの説明のためのソース
 
 ・IPv6に対応
*/

// gcc -Wall -o easyudpserver1 easyudpserver1.c

// socket用
#include <netinet/ip.h>
// read用
#include <unistd.h>
// perror用
#include <stdio.h>
// exit用
#include <stdlib.h>
// memset用
#include <string.h>
// signal用
#include <signal.h>
// getaddrinfo用
#include <netdb.h>


#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <jpeglib.h>

//! 待ち受けポート番号
#define MYPORT  50000


//--------------------------------------------------------------

/*! 
 @brief 待ち受けソケット作成
 
 @param[in] port    ポート番号
 @retval    0以上   待ち受けソケット番号
 @retval    -1      エラー
*/
int ore_listen( int port )
{
    int sd;                 // 待ち受けソケット
    struct addrinfo hints;  // bind用ヒント
    struct addrinfo *ai;    // bind用アドレス情報
    char portstr[16];
    int opt;
    int result;
    
    
    // listenポートの情報を設定
    memset(&hints,0,sizeof(hints));
    hints.ai_flags       = AI_PASSIVE | AI_ADDRCONFIG;  // 待ち受けの指定
    hints.ai_family      = AF_UNSPEC;   // IPv6/IPv4 両方
    hints.ai_socktype    = SOCK_DGRAM;  // UDP です
    snprintf( portstr, sizeof(portstr), "%d", port );   // 待ち受けポート番号を文字列にする
    result = getaddrinfo( NULL, portstr, &hints, &ai );
    if( result!=0 ){
        fprintf( stderr,"getaddrinfo: %s\n", gai_strerror(result) );
        return -1;
    }
    
    // 待ち受けソケット作成
    sd = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol );
    if( sd<0 ){
        perror("socket");
        freeaddrinfo(ai);
        return -1;
    }
    
    // ソケットオプション登録
    opt = 1;
    setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) );

    // バインド
    result = bind( sd, ai->ai_addr, ai->ai_addrlen );
    if( result<0 ){
        perror("bind");
        close(sd);
        freeaddrinfo(ai);
        return -1;
    }

    // 用が済んだのでアドレス情報を開放
    freeaddrinfo(ai);

    return sd;
}


/*! 
 @brief メインロジック部分
 
 とりあえず、入力された文字をそのままエコーする感じで
 @param[in] sd  ソケット
 @return    接続を維持するなら1を返す。正常に切断されたら0を、エラーなら-1を返す。
 @retval    1   接続を維持
 @retval    0   正常に切断検知
 @retval    -1  エラー
*/
void echosample( int sd )
{
    ssize_t len;
    char buf[1024];
    struct sockaddr_storage addr;   // ソケットアドレス
    socklen_t addrlen;              // ソケットアドレス長
    char host[256];
    int result;
    FILE	*fp;
	char 	*filename = "output.jpg";

    if ((fp = fopen(filename, "ab"))== NULL) {
		fprintf(stderr, "cannot open %s\n", filename);
		exit(EXIT_FAILURE);
	}
    // 読む
    addrlen = sizeof(addr);
    len = recvfrom( sd, buf, 1024, 0, (struct sockaddr*)&addr, &addrlen );//sizeof(buf)=1024
    if( len<0 ){
        perror("recvfrom");
        return;
    }

    // 相手の名前を表示（無くても構わない）
    result = getnameinfo( (struct sockaddr*)&addr, addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST );
    if( result!=0 ){
        fprintf( stderr,"getnameinfo: %s\n", gai_strerror(result) );
    }else{
        printf("connected by %s\n",host);
    }

    printf("%d:recv:%.*s\n",sd,len,buf);
    
    // 書く
    sendto( sd, buf, len, 0, (struct sockaddr*)&addr, addrlen );
    fprintf(fp,"%s",buf);//add in file
    fclose(fp);
    return;
}

void jpeg_showing(void){
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

	fp = fopen("output.jpeg","rb");
	jpeg_stdio_src(&cinfo,fp);

	jpeg_read_header(&cinfo,TRUE);//ここでSegmentationFault

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
}
//--------------------------------------------------------------

/*!
 @brief メインルーチン
*/
int main( int argc, char **argv )
{
    int sd;         // 通信ソケット

    signal(SIGPIPE, SIG_IGN);   // PIPE切断シグナルを無視する

    // 待ち受けソケット作成
    sd = ore_listen( MYPORT );
    if( sd<0 ){
        exit(1);
    }
    
    // メインロジック部分呼び出し
    for(;;){
        echosample(sd);
        //jpeg_showing();
    }
}



