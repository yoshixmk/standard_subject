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


//--------------------------------------------------------------

/*! 
 @brief 接続ソケット作成
 
 @param[in] host    ホスト名
 @param[in] port    ポート番号
 @retval    0以上   ソケット番号
 @retval    -1      エラー
*/
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
    			printf("%d:recv:%.*s\n",sd,1400,buf);
				printf("%ld \n",cnt1);
				printf("%x \n",buf[cnt1]);
			}
		
    // 読む   
    
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

	printf("Start!!!\n");
    open_device();		//　ビデオバイスをオープン
    init_device();		//　ビデオデバイスを初期化
    start_capturing();		//　画像キャプチャ開始

	    if( argc<3 ){
	        printf("usage: %s <ipaddress> <port>\n",argv[0]);
	        exit(0);
	    }
	    host = argv[1];
	    port = atoi(argv[2]);
	    
	    signal(SIGPIPE, SIG_IGN);   // PIPE切断シグナルを無視する

		/*while(deal != 1){
		    	mainloop();			//　メインループ処理
				for(;;)
				{
			        sd = ore_connect( host, port );
			        if( sd<0 ){
			            sleep(1);
			         	continue;
					}
			    

			        // メインロジック部分呼び出し
					for(;;){
			            result = sendsample(sd);
			            if( result<=0 || result ==1 ) break;
			            sleep(1);
			        }

				}*/
		   for(;;){
				 sd = ore_connect( host, port );
		        	if( sd>0 ){
		            break;
				}
					}
		while(deal != 1)
		{
			printf("\n\n cnt=%d \n\n",cnt++);
			if(cnt==3)return 0;
			printf("mainloop_in\n");
	    	mainloop();			//　メインループ処理
			printf("mainloop_end\n");
            sleep(1);
		        // メインロジック部分呼び出し
				for(;;){
		            result = sendsample(sd);
		            if( result<=0 || result ==1 ) break;
		            sleep(1);
		        }
			read(sd,show,8);
			while( strncmp(show,"show_end",8) != 0);
			printf("next_gazou\n");
        
        // クローズ
        printf("close\n");
        printf("-------------------------------------------------------------------------------------------\n");
		sleep(2);
        
    }
	    stop_capturing();		//　画像キャプチャ停止
	    uninit_device();		//　初期化前の状態に戻す
	    close_device();		//　ビデオデバイスをクローズ
		close(sd);
		return 0;
}

