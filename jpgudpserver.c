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
    char buf[256];
    struct sockaddr_storage addr;   // ソケットアドレス
    socklen_t addrlen;              // ソケットアドレス長
    char host[256];
    int result;
    
    
    // 読む
    addrlen = sizeof(addr);
    len = recvfrom( sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &addrlen );
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
    
    return;
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
    }
}



