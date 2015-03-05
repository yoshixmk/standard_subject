/*! 
 @file
 @brief もっとも簡単なサーバプログラム
 
 基本的なサーバプログラムの流れの説明のためのソース
*/

// gcc -Wall -o easyserver1 easyserver1.c

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
// inet_pton用
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <jpeglib.h>
#include <linux/input.h>

//! 待ち受けポート番号
#define MYPORT  50000
int hyouzi(); //プロトタイプ宣言
char bufgazou[50000];
unsigned int bb=0; 

//--------------------------------------------------------------

/*! 
 @brief 待ち受けソケット作成
 
 @param[in] port    ポート番号
 @retval    0以上   待ち受けソケット番号
 @retval    -1      エラー
*/
int ore_listen( int port )
{
    int sd;                     // 待ち受けソケット
    struct sockaddr_in addr;    // ソケットアドレス
    int opt;
    int result;
    

    // listenポートの情報を設定
    memset(&addr,0,sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);

    // 待ち受けソケット作成
    sd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( sd<0 ){
        perror("socket");
        return -1;
    }
    
    // ソケットオプション登録
    opt = 1;
    setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) );

    // バインド
    result = bind( sd, (struct sockaddr*)&addr, sizeof(addr) );
    if( result<0 ){
        perror("bind");
        close(sd);
        return -1;
    }

    // 待ち受け開始
    result = listen( sd, SOMAXCONN );
    if( result<0 ){
        perror("listen");
        close(sd);
        return -1;
    }
    
    return sd;
}


/*! 
 @brief 待ち受け処理
 
 接続を待ち受け、接続されたらソケットを返す
 @param[in] listen_sd   待ち受けソケット
 @retval    0以上   ソケット番号
 @retval    -1      エラー
*/
int ore_accept( int listen_sd )
{
    int sd;                         // ソケット
    struct sockaddr_in addr;        // ソケットアドレス
    socklen_t len;                  // ソケットアドレス長
    char host[256];                 // ホスト名表示用
    const char *p;
    
    
    // 待ち受け
    len = sizeof(addr);
    sd = accept( listen_sd, (struct sockaddr*)&addr, &len );
    if( sd<0 ){
        perror("accept");
        return -1;
    }
    
    // 相手の名前を表示（無くても構わない）
    p = inet_ntop( AF_INET, (void *)&addr.sin_addr, host, sizeof(host) );
    if( p==NULL ){
        perror("inet_ntop");
    }else{
        printf("connected by %s\n",host);
    }

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
int echosample( int sd )
{
    ssize_t len;
    char buf[1400];
	int cnt1 = 0;
	
    
    // 読む
    len = read( sd, buf, sizeof(buf) );
    if( len<0 ){
        perror("read");
        return -1;
    }
    if( len==0 ){
        return 0;
    }
    // 書く
    write( sd, buf, len);
		if(strncmp(buf,"abcdefg",7)==0)
			{
				while(bufgazou[bb-1]=='0'){
						bb--;	
					//printf("%d\n",bb);
				}return 2;

			}
		else
			{
				for(cnt1=0;cnt1<1400;cnt1++)
				{
					bufgazou[bb++]= buf[cnt1];
					
				}
			}
	
	return 1;
}

int ore_connect( char *host, int port )
{
    int sd;                     // 待ち受けソケット
    struct sockaddr_in addr;    // ソケットアドレス
    int result;


    // 接続先の情報を設定
    memset(&addr,0,sizeof(addr));
    result = inet_pton( AF_INET, host, &addr.sin_addr );
    if( result<0 ){
        perror("inet_pton");
        exit(0);
    }else if( result==0 ){
        printf("%s is not IP\n", host);
        exit(0);
    }
    addr.sin_family      = AF_INET;
    addr.sin_port        = ntohs(port);

    // 接続用ソケット作成
    sd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( sd<0 ){
        perror("socket");
        return -1;
    }

    // 接続開始
    result = connect(sd, (struct sockaddr*)&addr, sizeof(addr));
    if( result<0 ){
        perror("connect");
        close(sd);
        return -1;
    }
    
    return sd;
}

//--------------------------------------------------------------

/*!
 @brief メインルーチン
*/
int main( int argc, char **argv )
{	
	FILE *fp;
	char *filename = "output.jpg";
	char host[15] = "192.168.1.6";     // 接続先IPアドレス
    int port = 50000;
    int listen_sd;  // 待ち受けソケット
    int sd;         // 通信ソケット
    int result;
	
	/*
	  listen_sd = ore_listen( MYPORT );
    if( listen_sd<0 ){
        exit(1);
	}
	*/
	while(1){
	 sd = ore_connect( host, port );
        if( sd>0 ) break;  
    }

    signal(SIGPIPE, SIG_IGN);   // PIPE切断シグナルを無視する
   for(;;){
		fp=fopen(filename,"wb");
             // メインロジック部分呼び出し
        for(;;){
            result = echosample(sd);
			if( result<=0 || result == 2) break;
		}        
	    fwrite(bufgazou,sizeof(char),bb,fp);
		bb=0;
		fclose(fp);
		usleep(3000);
		hyouzi();
		write( sd,"show_end",8);		
	}
	close(sd);
	return 0;
}



int hyouzi(void){
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
	FILE *fa;
	int r,g,b;
	unsigned long pct;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	fa = fopen("output.jpg","rb");
	jpeg_stdio_src(&cinfo,fa);
	
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
	fclose(fa);
	for(i=0;i<height;i++)free(img[i]);
	free(img);
	return 0;
}



