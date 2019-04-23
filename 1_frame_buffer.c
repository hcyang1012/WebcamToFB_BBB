

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>

 #include <linux/fb.h>

int main() {
    const char *FB_NAME = "/dev/fb1";
    unsigned char*   m_FrameBuffer;
    struct  fb_var_screeninfo m_VarInfo;
    int 	m_FBFD;
    int iFrameBufferSize;
    int i = 0;
    int r = 0, g = 0, b = 0;

    /* Open the framebuffer device in read write */
    m_FBFD = open(FB_NAME, O_RDWR);
    if (m_FBFD < 0) {
    	printf("Unable to open %s.\n", FB_NAME);
    	return 1;
    }

    /* Do Ioctl. Get the variable screen info. */
    if (ioctl(m_FBFD, FBIOGET_VSCREENINFO, &m_VarInfo) < 0) {
    	printf("Unable to retrieve variable screen info: %s\n",
    		  strerror(errno));
    	close(m_FBFD);
    	return 1;
    }

    /* Calculate the size to mmap */
    iFrameBufferSize = m_VarInfo.xres * m_VarInfo.yres * (m_VarInfo.bits_per_pixel / 8);
    printf("Screen size : %dx%d\n", m_VarInfo.xres,m_VarInfo.yres);
    printf("BPS : %d\n",m_VarInfo.bits_per_pixel);

    /* Now mmap the framebuffer. */
    m_FrameBuffer = (unsigned char*)mmap(NULL, iFrameBufferSize, PROT_READ | PROT_WRITE,
    				 MAP_SHARED, m_FBFD,0);
    if (m_FrameBuffer == NULL) {
    	printf("mmap failed:\n");
    	close(m_FBFD);
    	return 1;
    }
    
    for(i = 0 ; i < iFrameBufferSize ; i += 2){
        unsigned short *p = (unsigned short*)&m_FrameBuffer[i];
        *p = (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | b & 0x1F);
        r = g = b = ((b + 1) % 255);
    }
    

}