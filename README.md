# ENSC351 3D Scanner

This repository implements an automated photogrammetry scanner built around a BeagleY-AI, a webcam, and two stepper-driven motion axes. The current codebase is more advanced than the earlier single-stepper, file-triggered version: it adds a modular scanner controller, a UDP server, a web UI, multiple scan modes, and a two-height capture workflow intended for higher-quality 3D reconstruction.

## Project overview

The scanner captures a sequence of overlapping images from multiple angles and then reuses them with COLMAP to reconstruct a 3D model. The system is designed around:

- a rotating platform driven by a stepper motor
- an adjustable camera height driven by a second stepper motor
- a webcam configured for MJPEG capture
- an NFS shared directory for transferring images to the host
- a control interface that sends scan commands over UDP

This is a production-oriented update relative to the older README, which described only a simpler single-stepper setup and a host trigger file flow.

## Current architecture

The newest code reorganizes the system into several modules:

- `hal/`: low-level hardware drivers for the camera and stepper controllers
- `app/src/scanner.c`: scan logic and scan-state handling
- `app/src/server.c`: UDP command server for mode changes and scan starts
- `app/src/main.c`: system startup and main loop
- `server/public/`: browser-based control panel for the scanner
- `server/server.js` and the Node helper files: host-side server/web integration

### Hardware flow

- Platform stepper: rotates the object for each capture
- Height stepper: changes the camera height between levels
- Camera: captures still images to the shared NFS folder
- Web/UI control: sends commands such as mode selection, pause/resume, start, and custom parameters

## Scan workflow

The active scan path in the current project does the following:

1. Start the scanner application on the BeagleY-AI
2. Start the network/web service used to control the device
3. Send a command to select a scan mode
4. Start the scan from the control interface
5. The app captures images at the configured rotation intervals and height levels
6. Each image is saved to `/mnt/nfs_share/myApps/scanNNN.jpg`
7. A host-side COLMAP pipeline can then ingest those images and generate the 3D reconstruction

The current code still writes `done.txt` to the shared folder as a completion signal, but the main operational control path is now the UDP server instead of a polling `start_scan.txt` file.

## Supported modes

The newer scanner supports these modes defined in `hal/include/scanner.h`:

- `STANDARD_AUTO` (`0`): standard single-height scan
- `DETAILED_AUTO` (`1`): multi-height scan with additional height steps
- `CUSTOM_MODE` (`2`): custom parameters for sample count, height change, and number of heights

Default values include:

- `DEFAULT_SAMPLE_PER_REV = 20`
- `DEFAULT_HEIGHT_CHANGE = 30`
- `SD_NUMBER_OF_HEIGHTS = 1`
- `DT_NUMBER_OF_HEIGHTS = 2`

The scanner also supports custom configuration values through the UDP command string pattern:

```text
c <sample_per_rev> <height_change> <num_of_heights>
```

The current web/server flow uses commands like:

```text
mode 0
mode 1
mode 2
start
pause toggle
stop
shutdown
```

## Repository structure

```text
.
├── CMakeLists.txt
├── README.md
├── app/
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.c
│       ├── scanner.c
│       └── server.c
├── hal/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── camera.h
│   │   ├── height_stepper.h
│   │   ├── platform_stepper.h
│   │   ├── scanner.h
│   │   ├── server.h
│   │   └── stepper.h
│   └── src/
│       ├── camera.c
│       ├── height_stepper.c
│       ├── platform_stepper.c
│       └── stepper.c
├── server/
│   ├── fake_scanner_udp
│   ├── fake_scanner_udp.c
│   ├── lib/
│   ├── package.json
│   ├── public/
│   └── server.js
├── build/
└── documents/
```

## Requirements

### Host machine

Install the host-side tools needed for image processing and shared storage:

```bash
sudo apt install colmap
sudo apt install nfs-kernel-server
```

### BeagleY-AI

Install the required C and camera libraries:

```bash
sudo apt install -y libgpiod-dev libv4l-dev build-essential cmake
sudo apt install nfs-common
```

Common dependencies include:

- `libgpiod-dev` for GPIO access and motor control
- `libv4l-dev` for webcam capture via V4L2
- `build-essential` for the C toolchain
- `cmake` for building the project

## Shared folder and file paths

The code assumes the images are stored in the shared NFS directory:

```text
/mnt/nfs_share/myApps
```

The main constants are defined in `hal/include/scanner.h` and include:

- `NFS_PATH` - image output location
- `CMD_FILE` - legacy trigger file path
- `DONE_FILE` - scan completion marker

The camera device in the current version is configured in `hal/src/camera.c` and is typically `/dev/video0` depending on hardware detection.

## Building the project

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

If you use VS Code with the CMake Tools extension, you can also build through the editor UI.

## Running the scanner

### On the BeagleY-AI

After building:

```bash
./build/app/3DScanner
```

The current C application initializes the camera, starts the UDP server, and waits for scan commands.

### Control commands

The scanner is operated through the UDP server and browser UI rather than only through a local shell interface. Typical commands are:

```text
mode 0
mode 1
mode 2
start
pause toggle
stop
c 20 30 2
```

A browser page under `server/public/` can be used to interact with the scanner once the server service is running.

## Troubleshooting

### Camera detection

Check what camera devices are available:

```bash
ls -l /dev/video*
```

Inspect device capabilities:

```bash
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 --list-formats-ext
```

If your board uses a different webcam device, update the `dev_name` value in `hal/src/camera.c`.

### GPIO and stepper verification

Confirm the board GPIO chips and pin mapping:

```bash
gpiodetect
gpioinfo gpiochip0
```

This is important because the steppers and the scanning sequence depend on the correct GPIO layer and pin assignments.

### NFS setup

The project expects a working NFS share between the BeagleY-AI and the host. If the path differs from `/mnt/nfs_share/myApps`, update the path constants in the relevant headers and source files before building.

