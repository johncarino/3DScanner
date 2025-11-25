/* camera.c */
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
#include "camera.h"

// Define how many frames to capture and discard before saving the good one
#define SKIP_FRAMES 5
#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
        void   *start;
        size_t length;
};

// File-static variables to hold camera state
static int                      fd = -1;
static struct buffer            *buffers;
static unsigned int             n_buffers;
static char                     *dev_name = "/dev/video3"; 

static void xioctl(int fh, unsigned long request, void *arg)
{
        int r;
        do {
                r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

int camera_init(void) 
{
        struct v4l2_format              fmt;
        struct v4l2_requestbuffers      req;
        struct v4l2_buffer              buf;
        enum v4l2_buf_type              type;

        // Open the device (Using /dev/video3 as per your latest file)
        fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
                perror("Cannot open device");
                return -1;
        }

        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = 1920;
        fmt.fmt.pix.height      = 1080;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;
        xioctl(fd, VIDIOC_S_FMT, &fmt);
        
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG) {
                printf("Libv4l didn't accept MJPEG format. Can't proceed.\n");
                return -1;
        }

        CLEAR(req);
        req.count = 8;
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
                buffers[n_buffers].start = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
                if (MAP_FAILED == buffers[n_buffers].start) {
                        perror("mmap");
                        return -1;
                }
        }

        for (unsigned int i = 0; i < n_buffers; ++i) {
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                xioctl(fd, VIDIOC_QBUF, &buf);
        }
        
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMON, &type);
        printf("Camera initialized and stream is ON.\n");
        return 0;
}

int camera_capture_one_image(const char *folder_path, int frame_num)
{
        struct v4l2_buffer              buf;
        fd_set                          fds;
        struct timeval                  tv;
        int                             r;
        char                            out_name[512];
        FILE                            *fout;

        if (fd < 0) {
            fprintf(stderr, "Camera not initialized.\n");
            return -1;
        }

        // --- LOOP TO FLUSH BUFFER ---
        // We capture 'SKIP_FRAMES' times but only save the very last one.
        for (int i = 0; i < SKIP_FRAMES; i++) {

                do {
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);
                        tv.tv_sec = 2; // 2 second timeout
                        tv.tv_usec = 0;
                        r = select(fd + 1, &fds, NULL, NULL, &tv);
                } while ((r == -1 && (errno = EINTR)));
                
                if (r == -1) {
                        perror("select");
                        return -1;
                }

                // Dequeue the buffer (Get image from camera)
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                xioctl(fd, VIDIOC_DQBUF, &buf);

                // --- ONLY SAVE IF THIS IS THE LAST FRAME ---
                if (i == SKIP_FRAMES - 1) {
                        snprintf(out_name, sizeof(out_name), "%s/scan%03d.jpg", folder_path, frame_num);
                        
                        fout = fopen(out_name, "w");
                        if (!fout) {
                                fprintf(stderr, "Cannot open image for writing at: %s\n", out_name);
                                perror("File open error");
                                // Re-queue buffer before returning failure
                                xioctl(fd, VIDIOC_QBUF, &buf); 
                                return -1;
                        }
                        
                        fwrite(buffers[buf.index].start, buf.bytesused, 1, fout);
                        fclose(fout);
                        printf("Saved %s (Discarded %d old frames)\n", out_name, i);
                }

                // Re-queue the buffer (Give it back to the camera for the next shot)
                xioctl(fd, VIDIOC_QBUF, &buf); 
        }
        
        return 0;
}

void camera_cleanup(void)
{
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMOFF, &type);

        for (unsigned int i = 0; i < n_buffers; ++i)
                v4l2_munmap(buffers[i].start, buffers[i].length);
        
        v4l2_close(fd);
        free(buffers);
        printf("Camera cleaned up.\n");
}