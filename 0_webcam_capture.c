/* V4L2 video picture grabber
* Copyright (C) 2009 Mauro Carvalho Chehab <mchehab@infradead.org>
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2 of the License.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* Modified by Derek Molloy (www.derekmolloy.ie)
* Modified to change resolution details and set paths for the Beaglebone.

* Modified by Heecheol Yang (heecheol.yang@outlook.com)
* Modified to change color format suitable for my webcam
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
  void   *start;
  size_t length;
};

static void xioctl(int fh, int request, void *arg)
{
  int r;

  do {
    r = ioctl(fh, request, arg);
    
  } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

  if (r == -1) {
    fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

/* 
*  Convert YUV422 buffer to RGB888
*  Bring code from https://gist.github.com/crouchggj/6894292
*  Thanks a lot!, guanguojin!
*/
int convert_yuv_to_rgb_pixel(int y, int u, int v)
{
  unsigned int pixel32 = 0;
  unsigned char *pixel = (unsigned char *)&pixel32;
  int r, g, b;
  r = y + (1.370705 * (v-128));
  g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
  b = y + (1.732446 * (u-128));
  if(r > 255) r = 255;
  if(g > 255) g = 255;
  if(b > 255) b = 255;
  if(r < 0) r = 0;
  if(g < 0) g = 0;
  if(b < 0) b = 0;
  pixel[0] = r ;
  pixel[1] = g ;
  pixel[2] = b ;
  return pixel32;
}

int convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
  unsigned int in, out = 0;
  unsigned int pixel_16;
  unsigned char pixel_24[3];
  unsigned int pixel32;
  int y0, u, y1, v;

  for(in = 0; in < width * height * 2; in += 4)
  {
    pixel_16 =
    yuv[in + 3] << 24 |
    yuv[in + 2] << 16 |
    yuv[in + 1] <<  8 |
    yuv[in + 0];
    y0 = (pixel_16 & 0x000000ff);
    u  = (pixel_16 & 0x0000ff00) >>  8;
    y1 = (pixel_16 & 0x00ff0000) >> 16;
    v  = (pixel_16 & 0xff000000) >> 24;
    pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
    pixel_24[0] = (pixel32 & 0x000000ff);
    pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
    pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
    rgb[out++] = pixel_24[0];
    rgb[out++] = pixel_24[1];
    rgb[out++] = pixel_24[2];
    pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
    pixel_24[0] = (pixel32 & 0x000000ff);
    pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
    pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
    rgb[out++] = pixel_24[0];
    rgb[out++] = pixel_24[1];
    rgb[out++] = pixel_24[2];
  }
  return 0;

}

int main(int argc, char **argv)
{
  struct v4l2_format              fmt;
  struct v4l2_buffer              buf;
  struct v4l2_requestbuffers      req;
  enum v4l2_buf_type              type;
  fd_set                          fds;
  struct timeval                  tv;
  int                             r, fd = -1;
  unsigned int                    i, n_buffers;
  char                            *dev_name = "/dev/video0";
  char                            out_name[256];
  FILE                            *fout;
  struct buffer                   *buffers;
  char                            *rgb_buffer;
  const int                       width = 640;
  const int                       height = 480;
  const enum v4l2_field           pixelformat = V4L2_PIX_FMT_YUYV;

  // Open /dev/video0
  fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
  if (fd < 0) {
    perror("Cannot open device");
    exit(EXIT_FAILURE);
  }

  CLEAR(fmt);

  
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = width;
  fmt.fmt.pix.height      = height;
  fmt.fmt.pix.pixelformat = pixelformat;


  fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
  xioctl(fd, VIDIOC_S_FMT, &fmt);
  if (fmt.fmt.pix.pixelformat != pixelformat) {
    printf("Libv4l didn't accept this color format. Can't proceed.\n");
    exit(EXIT_FAILURE);
  }
  if ((fmt.fmt.pix.width != width) || (fmt.fmt.pix.height != height))
    printf("Warning: driver is sending image at %dx%d,but you requested %dx%d\n",
      fmt.fmt.pix.width, fmt.fmt.pix.height,width,height);

  CLEAR(req);
  req.count = 2;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  xioctl(fd, VIDIOC_REQBUFS, &req);

  buffers = calloc(req.count, sizeof(*buffers));
  for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
    CLEAR(buf);

    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = n_buffers;
    xioctl(fd, VIDIOC_QUERYBUF, &buf);

    buffers[n_buffers].length = buf.length;
    buffers[n_buffers].start = mmap(NULL, buf.length,
      PROT_READ | PROT_WRITE, MAP_SHARED,
      fd, buf.m.offset);

    if (MAP_FAILED == buffers[n_buffers].start) {
      perror("mmap");
      exit(EXIT_FAILURE);
    }
  }

  for (i = 0; i < n_buffers; ++i) {
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    xioctl(fd, VIDIOC_QBUF, &buf);
  }

  rgb_buffer = (char*)malloc(sizeof(char) * width * height * 2);
  printf("reb_buffer size : %d\n",width * height * 2);
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(fd, VIDIOC_STREAMON, &type);
  for (i = 0; i < 20; i++) {
    do {
      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      /* Timeout. */
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      r = select(fd + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno = EINTR)));
    if (r == -1) {
      perror("select");
      return errno;
    }

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_DQBUF, &buf);

    sprintf(out_name, "frame%03d.bin", i);
    fout = fopen(out_name, "w");
    if (!fout) {
      perror("Cannot open image");
      exit(EXIT_FAILURE);
    }

    convert_yuv_to_rgb_buffer(buffers[buf.index].start,rgb_buffer,width,height);
    fwrite(rgb_buffer, buf.bytesused, 1, fout);
    printf("Write size : %d\n",buf.bytesused);
    fclose(fout);

    xioctl(fd, VIDIOC_QBUF, &buf);
  }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(fd, VIDIOC_STREAMOFF, &type);
  for (i = 0; i < n_buffers; ++i)
    munmap(buffers[i].start, buffers[i].length);
  close(fd);

  return 0;
}

