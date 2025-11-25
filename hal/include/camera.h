// based on the boneCV repository
// takes a single photo

#ifndef CAMERA_H
#define CAMERA_H

// Initializes the /dev/video0 device and starts the stream
// Returns 0 on success, -1 on failure.
int camera_init(void);

// Captures a single image and saves it.
// frame_num: Used to create the filename (e.g., "scan001.jpg")
int camera_capture_one_image(const char *folder_path,int frame_num);

// Stops the stream and closes the device
void camera_cleanup(void);

#endif