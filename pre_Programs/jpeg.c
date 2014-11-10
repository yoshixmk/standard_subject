#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <jpeglib.h>

int main(void){
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
	return 0;
}
