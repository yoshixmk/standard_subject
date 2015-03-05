/*! 
 @file
 @brief もっとも簡単なクライアントプログラム
 
 基本的なクライアントプログラムの流れの説明のためのソース
*/

// gcc -Wall -o easyclient1 easyclient1.c

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
#include "douga.h"
//! 待ち受けポート番号
#define MYPORT  50000

//--------------------------------------------------------------
/*! 
 @brief 接続ソケット作成
 
 @param[in] host    ホスト名
 @param[in] port    ポート番号
 @retval    0以上   ソケット番号
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
 
 20文字の文字列を送信し、受信した文字列を出力する
 @param[in] sd  ソケット
 @return    接続を維持するなら1を返す。正常に切断されたら0を、エラーなら-1を返す。
 @retval    1   接続を維持
 @retval    0   正常に切断検知
 @retval    -1  エラー
*/
int sendsample( int sd )
{
    
	FILE *fp;
	fpos_t pos, endpos;
    int result;
	char buf[2000][1400];
	int cnt1 = 0, cnt2 =0;
	int nokori;
	int bb;
	int c;
	int len=0;
	fp = fopen("./output.jpg", "rb");fgetpos(fp, &pos);
	fseek(fp, 0L, SEEK_END);
	if(fp == NULL){
		printf("OPEN_ERR\n");
	}
	printf("pos: %ld\n", pos);
	fgetpos(fp, &endpos);
	fsetpos(fp, &pos);
	printf("endpos: %ld\n", endpos);
	printf("pos: %ld\n", pos);
	bb = endpos.__pos;

	while(bb!=0){
		if(bb >= 1400){
		fread(buf[cnt1],sizeof(char),1400,fp);
		bb = bb -1400;
		cnt1++;
		}
		else
		{
			fread(buf[cnt1],sizeof(char),bb,fp);
			nokori=1400-bb;
			c=bb;
			for(nokori;nokori>0;nokori--){
			buf[cnt1][c]='0';
			c++;
			}
			bb=0;
			c=0;
			cnt1++;
			strcpy(buf[cnt1],"abcdefg");
			cnt1++;
			strcpy(buf[cnt1],"aaaaa");
		}

	}
			for(cnt1=0;cnt1<1000;cnt1++){
				if(strncmp(buf[cnt1],"aaaaa",5)==0)break;
				result = write( sd, buf[cnt1], 1400 );
				//if(strncmp(buf[cnt1],"abcdefg",7)==0)break;
   						 if( result==0 ){
   							return 0;
						  }
						else if( result<0 ){
   					     		perror("write");
   					     		return -1;
						}

				 result = read( sd, buf,1400 );
    					if( result==0 ){
        					return 0;
    					}else if( result<0 ){
        				perror("read");
        				return -1;
    				}
    			//printf("%d:recv:%.*s\n",sd,1400,buf);
				//printf("%ld \n",cnt1);
				//printf("%x \n",buf[cnt1]);
			}
    fclose(fp);
    return 1;
}

//--------------------------------------------------------------

/*!
 @brief メインルーチン
*/
int main( int argc, char **argv )
{
    int sd;         // 通信ソケット
    int result;
    char *host;     // 接続先IPアドレス
    int port;       // 接続先ポート番号
	char show[10];
	int cnt;
	int listen_sd;

	printf("Start!!!\n");
    open_device();		//　ビデオバイスをオープン
    init_device();		//　ビデオデバイスを初期化
    start_capturing();		//　画像キャプチャ開始
	/*
    if( argc<3 ){
        printf("usage: %s <ipaddress> <port>\n",argv[0]);
        exit(0);
    }
	
    host = argv[1];
    port = atoi(argv[2]);
    */
    signal(SIGPIPE, SIG_IGN);   // PIPE切断シグナルを無視する
	 /*
	for(;;){
	sd = ore_connect( host, port );
        if( sd>0 ){
            	break;
		}
	}
	*/
    // 待ち受けソケット作成
    listen_sd = ore_listen( MYPORT );
    if( listen_sd<0 ){
        exit(1);
    }
	for(;;){
	printf("waiting\n");
	sd = ore_accept( listen_sd );
        if( sd>0 ){
            	break;
		}
	}
	while(deal != 1)
	{
		printf("mainloop_in\n");
    	mainloop();			//　メインループ処理
		printf("mainloop_end\n");
        usleep(1000);
	        // メインロジック部分呼び出し
			//for(;;){
	    result = sendsample(sd);

	    	    //  if( result<=0 || result ==1 ) break;
	    usleep(1000);
	        //}
		for(;;){
		read(sd,show,8);
			if( strncmp(show,"show_end",8) == 0);
			{
				printf("show OK!!\n");
				show[0]='0';
				break;
			}
		}
		printf("next_gazou\n");


	    // クローズ
	    printf("close\n");
	    printf("-------------------------------------------------------------------------------------------\n");
	 	usleep(1000);
	}
    stop_capturing();		//　画像キャプチャ停止
    uninit_device();		//　初期化前の状態に戻す
    close_device();		//　ビデオデバイスをクローズ
	close(sd);
	return 0;
}