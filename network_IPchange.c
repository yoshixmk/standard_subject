#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>

int parse_form(char*s_in, long maxl, char* name[], char* value[], int *p_nfld);

int main(void)
{
    int status1=0;
    char *ev;
    char *cqString;
    char* name[10];
    char* value[10];
    int nfld, i, size;
    int fd, ret, status;
    char buf[256];
    char str[256]="auto lo eth0\niface lo inet loopback\niface eth0 inet static\naddress ";
    char str2[128]="\nnetmask 255.255.255.0\nnetwork 192.168.1.0\nbroadcast 192.168.1.255\ngateway 192.168.1.254\n";
    //system("echo "" > /etc/network/interfaces");
    
    fd=open("/etc/network/interfaces", O_WRONLY);
    if(fd==-1){
        perror("open");
        return EXIT_FAILURE;
    }

    ev=getenv("REQUEST_METHOD");
    cqString=getenv("QUERY_STRING");
    parse_form(cqString, 128, name, value, &nfld);
    size = strlen(str);
    for(i=0; i < strlen(value[0]); i++){
        str[size+i] = value[0][i];
    }
    size += strlen(value[0]);
    for(i=0; i<strlen(str2); i++){
        str[size+i] = str2[i];
    }
    size += strlen(str2);
    ret=write(fd, str, size);

    close(fd);
    system("sudo ifdown eth0; sudo ifup eth0");

    return 0;
}

int parse_form(char*s_in, long maxl, char* name[], char* value[], int *p_nfld)
{
    int i, cur_field;
    *p_nfld = 0;
    i=0;
    cur_field = 0;
    name[0] = s_in;

    while((s_in[++i] != '\0')&&(i<maxl)){
        if(s_in[i] == '='){
            s_in[i]='\0';
            value[cur_field]=s_in+i+1;
        }
        else if(s_in[i]=='&'){
            s_in[i]='\0';
            cur_field++;
            name[cur_field]=s_in+i+1;
        }
    }
    *p_nfld = cur_field+1;
    return 0;
}



