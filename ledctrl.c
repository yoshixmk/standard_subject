#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

#define CLASS "/sys/class/leds"

void html_print(int led[]);
int led_status(int led[]);
int led_out(int led[]);
void html_error(char *str);
int parse_form(char* s_in,long maxl,char* name[],char* value[],int *p_nfld);
int decode_form(char* s,long len);
int get_form(int led[]);

enum LED_COLOR {
	RED,
	GREEN,
	YELLOW,
};

int main(int argc, char *argv[]){
	int ret;
	int led[3];

	ret = led_status(led);
	if( ret == -1 ) {
		html_error("status");
		return -1;
	}
	ret = get_form(led);
	if( ret == -1 ) {
		html_error("get_form");
	}
	ret = led_out(led);
	if( ret == -1 ) {
		html_error("led_out");
		return -3;
	}
	html_print(led);
	return 0;
}
int led_out(int led[])
{
	int fd;
	int ret;
	char buf[16];
	char classpath[256];
	char c_name[3][8] = {"red","green","yellow"};
	int i;

	for( i=RED; i<=YELLOW; i++ ) {
		sprintf( classpath, "%s/%s/brightness", CLASS, c_name[i] );
		fd = open(classpath, O_WRONLY);
		if(fd == -1){
			perror("open");
			return -1;
		}
		sprintf( buf, "%d", led[i] );
		ret = write(fd,buf,strlen(buf));
		if(ret == -1){
			perror("write");
			close(fd);
			return -1;
		}
		close(fd);
	}
	return 0;
}

int led_status(int led[])
{
	int fd;
	int ret;
	char buf[16];
	char classpath[256];
	char c_name[3][8] = {"red","green","yellow"};
	int i;

	for( i=RED; i<=YELLOW; i++ ) {
		sprintf( classpath, "%s/%s/brightness", CLASS, c_name[i] );
		fd = open(classpath, O_RDONLY);
		if(fd == -1){
			perror("open");
			return -1;
		}
		ret = read(fd,buf,256);
		if(ret == -1){
			perror("read");
			close(fd);
			return -1;
		}
		close(fd);
		led[i] = strtol(buf, NULL, 0);
	}
	return 0;
}

void html_print(int led[])
{
	int i;
	char c_name[3][8] = {"LED_R_","LED_G_","LED_Y_"};
	char on_off[2][10] = {"OFF.jpg", "ON.jpg"};
        printf("Content-type: text/html\n\n");
        printf("<HTML><HEAD><TITLE>LED Control Status</TITLE></HEAD><BODY>\n");
	printf("<H1>LED Control STATUS</H1>\n");
	for( i=RED; i<=YELLOW; i++ ) {
		printf("<img src=\"image/%s%s\">%d\n", c_name[i], on_off[led[i]], led[i] ); 
	}
        printf("</BODY></HTML>\n");
	return;
}

void html_error(char *str)
{
        printf("Content-type: text/html\n\n");
        printf("<HTML><HEAD><TITLE>LED Control ERROR</TITLE></HEAD><BODY>\n");
	printf("<H1>LED Control(%s) ERROR!!!</H1>\n", str);
        printf("</BODY></HTML>\n");
	return;
}

int get_form(int led[])
{
        char *inputstring,*cLength,*name[20],*value[20];
        int nfield;
        long i,length,MAXLEN=4096;

	if( !strcmp(getenv("REQUEST_METHOD"), "POST") ) {
		cLength=(char*)getenv("CONTENT_LENGTH");
		length=atol(cLength);
		if(length>MAXLEN) {printf("CONTENT_LENGTH>MAXLEN\n");return -1;}
		inputstring=(char*)malloc(length+1);
		scanf("%s",inputstring);
	} else if( !strcmp(getenv("REQUEST_METHOD"), "GET") ) {
		length=strlen((char *)getenv("QUERY_STRING"));
		if(length>MAXLEN) {printf("CONTENT_LENGTH>MAXLEN\n");return -1;}
		inputstring=(char*)malloc(length+1);
		strcpy(inputstring, getenv("QUERY_STRING"));
	}
	if(inputstring == NULL ) {
		return -1;
	}
        parse_form(inputstring,MAXLEN,name,value,&nfield);
        for(i=0;i<nfield;i++){
                decode_form(name[i],strlen(name[i]));
                decode_form(value[i],strlen(value[i]));
		if( !strcmp(name[i], "RED" )) {
			if( !strcmp(value[i], "ON" )) {
				led[RED] = 1;
			} else if( !strcmp(value[i], "OFF" )) {
				led[RED] = 0;
			}
		} else if( !strcmp(name[i], "GREEN" )) {
			if( !strcmp(value[i], "ON" )) {
				led[GREEN] = 1;
			} else if( !strcmp(value[i], "OFF" )) {
				led[GREEN] = 0;
			}
		} else if( !strcmp(name[i], "YELLOW" )) {
			if( !strcmp(value[i], "ON" )) {
				led[YELLOW] = 1;
			} else if( !strcmp(value[i], "OFF" )) {
				led[YELLOW] = 0;
			}
		}
        }
        free(inputstring);
	return 0;
}
int parse_form(char* s_in,long maxl,char* name[],char* value[],int *p_nfld)
{
        int i,cur_field;
        *p_nfld=0;
        i=0;
        cur_field=0;
        name[0]=s_in;
        while((s_in[++i]!='\0')&&(i<maxl)){
                if(s_in[i]=='='){
                        s_in[i]='\0';
                        value[cur_field]=s_in+i+1;
                }
                else if(s_in[i]=='&'){
                        s_in[i]='\0';
                        cur_field++;
                        name[cur_field]=s_in+i+1;
                }
        }
        *p_nfld=cur_field+1;
        return(0);
}

int decode_form(char* s,long len)
{
        int i,j;
        char buf,*s1;
        if(len==0)return(-1);
        s1=(char*)malloc(len);
        for(i=0,j=0;i<len;i++,j++)
        {
                if(s[i]=='+'){s1[j]=' ';continue;}
                if(s[i]!='%') {s1[j]=s[i];continue;}
                buf=((s[++i]>='A') ? s[i]-'A'+10 : s[i]-'0');
                buf*=16;
                buf+=((s[++i]>='A') ? s[i]-'A'+10 : s[i]-'0');
                s1[j]=buf;
        }
        for(i=0;i<j;i++) s[i]=s1[i];
        s[i]='\0';
        free(s1);
        return(0);
}
