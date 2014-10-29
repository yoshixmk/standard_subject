/*! 
 @file
 @brief もっとも簡単なクライアントプログラム
 
 基本的なクライアントプログラムの流れの説明のためのソース
 
 ・IPv6に対応
*/

// gcc -Wall -o easyudpclient1 easyudpclient1.c

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
    
    if( argc<3 ){
        printf("usage: %s <ipaddress> <port>\n",argv[0]);
        exit(0);
    }
    host = argv[1];
    port = atoi(argv[2]);
    
    signal(SIGPIPE, SIG_IGN);   // PIPE切断シグナルを無視する
    
    for(;;){
        sd = ore_connect( host, port );
        if( sd<0 ){
            sleep(1);
            continue;
        }

        // メインロジック部分呼び出し
        for(;;){
            result = sendsample(sd);
            sleep(1);
            if( result<=0 ) break;
        }
        
        // クローズ
        printf("close\n");
        close(sd);
    }
}



