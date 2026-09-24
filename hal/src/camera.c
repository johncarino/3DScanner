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
#define SKIP_FRAMES 10
#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
        void   *start;
        size_t length;
};

// File-static variables to hold camera state
static int                      fd = -1;
static struct buffer            *buffers;
static unsigned int             n_buffers;

// this changes value when you unplug and replug the webcam, so double check this
static char                     *dev_name = "/dev/video0"; 

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

        // needed to save the photo as a compressed format
        // saving a raw photo takes up too many resources on the beagleboard and bogs it down
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

        // the following settings were found with v4l2-ctl -d /dev/video0 --list-ctrls
        // these commands are camera specific so if using a different webcam, run the command again and change the correct values
        // a good image depends on the room lighting environment so nailing these takes a lot of trial and error
        struct v4l2_control ctrl;
        
        // BRIGHTNESS 
        ctrl.id = V4L2_CID_BRIGHTNESS;
        ctrl.value = 100; // min=0 max=255 step=1 default=128
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set brightness\n");} 
        else {printf("Brightness set to: %d\n", ctrl.value);}

        // CONTRAST 
        ctrl.id = V4L2_CID_CONTRAST;
        ctrl.value = 128; // min=0 max=255 step=1 default=128
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set contrast\n");} 
        else {printf("Contrast set to: %d\n", ctrl.value);}

        // SATURATION 
        ctrl.id = V4L2_CID_SATURATION;
        ctrl.value = 128; // min=0 max=255 step=1 default=128
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set saturation\n");} 
        else {printf("Saturation set to: %d\n", ctrl.value);}

        // WHITE BALANCE AUTOMATIC 
        ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
        ctrl.value = 1; // 0=manual, 1=auto, default=1
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set auto white balance\n");} 
        else {printf("Auto white balance set to: %d\n", ctrl.value);}

        // GAIN 
        ctrl.id = V4L2_CID_GAIN;
        ctrl.value = 0; // min=0 max=255 step=1 default=0
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set gain\n");} 
        else {printf("Gain set to: %d\n", ctrl.value);}

        // SHARPNESS 
        ctrl.id = V4L2_CID_SHARPNESS;
        ctrl.value = 150; // min=0 max=255 step=1 default=128
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set sharpness\n");} 
        else {printf("Sharpness set to: %d\n", ctrl.value);}

        // BACKLIGHT COMPENSATION 
        ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
        ctrl.value = 0; // min=0 max=1 step=1 default=0
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set backlight compensation\n");} 
        else {printf("Backlight compensation set to: %d\n", ctrl.value);}

        // AUTO EXPOSURE 
        ctrl.id = V4L2_CID_EXPOSURE_AUTO;
        ctrl.value = 3; // 0=auto, 1=manual, 3=aperture priority, default=3
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set auto exposure\n");} 
        else {printf("Auto exposure set to: %d\n", ctrl.value);}

        // PAN ABSOLUTE
        ctrl.id = V4L2_CID_PAN_ABSOLUTE;
        ctrl.value = 0; // min=-36000 max=36000 step=3600 default=0
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set pan\n");} 
        else {printf("Pan absolute set to: %d\n", ctrl.value);}

        // TILT ABSOLUTE 
        ctrl.id = V4L2_CID_TILT_ABSOLUTE;
        ctrl.value = 0; // min=-36000 max=36000 step=3600 default=0
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set tilt\n");} 
        else {printf("Tilt absolute set to: %d\n", ctrl.value);}

        // FOCUS ABSOLUTE
        ctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
        ctrl.value = 27; // min=0 max=250 step=5 default=0
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set focus\n");} 
        else {printf("Focus absolute set to: %d\n", ctrl.value);}

        // FOCUS AUTOMATIC CONTINUOUS
        ctrl.id = V4L2_CID_FOCUS_AUTO;
        ctrl.value = 0; // 0=manual continuous focus, 1=auto continuous focus, default=1
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set auto focus\n");} 
        else {printf("Focus automatic continuous set to: %d\n", ctrl.value);}

        // ZOOM ABSOLUTE
        ctrl.id = V4L2_CID_ZOOM_ABSOLUTE;
        ctrl.value = 100; // min=100 max=500 step=1 default=100
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {printf("Warning: Could not set zoom\n");} 
        else {printf("Zoom absolute set to: %d\n", ctrl.value);}

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

        // we capture 'SKIP_FRAMES' times but only save the very last one
        // this is so we let the camera's exposure to settle in, ideally giving a non blurry image
        for (int i = 0; i < SKIP_FRAMES; i++) {
                do {
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;
                        r = select(fd + 1, &fds, NULL, NULL, &tv);
                } while ((r == -1 && (errno = EINTR)));
                
                if (r == -1) {
                        perror("select");
                        return -1;
                }

                // dequeue the buffer get image from camera)
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                xioctl(fd, VIDIOC_DQBUF, &buf);

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

// this is specifically for the two focus values on a scan that requires two heights.
int camera_set_focus(int focus_value)
{
        struct v4l2_control ctrl;
        if (fd < 0) {
                fprintf(stderr, "Camera not initialized.\n");
                return -1;
        }
        
        // Clamp focus value to valid range (0-250, step=5)
        if (focus_value < 0) focus_value = 0;
        if (focus_value > 250) focus_value = 250;
        focus_value = (focus_value / 5) * 5; // Round to nearest multiple of 5
        
        ctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
        ctrl.value = focus_value;
        
        if (v4l2_ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {
                printf("Warning: Could not adjust focus\n");
                return -1;
        }
        
        printf("Focus adjusted to: %d\n", ctrl.value);
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
