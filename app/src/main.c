#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h> 
#include <stdbool.h>
#include "stepper.h"
#include "camera.h"
#include "scanner.h"
#include "platform_stepper.h"
#include "height_stepper.h"
#include "server.h"

#define STEPS_PER_ROTATION 3200
#define TOTAL_PICTURES 20

bool programRunning;

void signal_host_complete();
int test_stepper_only(void);
int test_camera_only(void);
int run_manual_timed_sequence(void);
int run_auto_listening_mode(void);
static int perform_scan_sequence(void);

// have a two terminals logged into the BeagleY-AI, one to start the server, the other to start the C-app
// run the python script on a seperate terminal, on the HOST MACHINE to detect and allow COLMAP to process the photos

// COLMAP requires a beefy GPU to process a point cloud from the pictures
// the time it takes to create said point cloud depends on the # of photos and the quality of the photos   

int test_stepper_only(void) {
    printf("\n[TEST] Starting Stepper Motor Test...\n");
    printf("Moving 400 steps forward...\n");
    stepper_move_and_wait(400);
    sleep(1);
    
    printf("Moving 400 steps backward...\n");
    stepper_move_and_wait(-400);
    
    printf("[TEST] Stepper test complete.\n");
    return 0;
}

int test_camera_only(void) {
    printf("\n[TEST] Starting Single Camera Capture Test...\n");
    printf("Capturing image scan999.jpg to %s...\n", NFS_PATH);
    
    if (camera_capture_one_image(NFS_PATH, 999) == 0) {
        printf("[TEST] Capture success! Check folder for scan999.jpg\n");
    } else {
        fprintf(stderr, "[TEST] Capture failed.\n");
        return -1;
    }
    return 0;
}

// Takes photos on a timer so you can move the cam by hand.
int run_manual_timed_sequence(void) {
    printf("\n--- Starting MANUAL Scan Sequence ---\n");
    printf("You have 9 seconds between shots to move the camera manually.\n");
    
    for (int i = 0; i < TOTAL_PICTURES; i++) {
        printf("\n[Move Camera Now] ... \n9 seconds remaining\n");
        sleep(1); printf("8...\n");
        sleep(1); printf("7...\n");
        sleep(1); printf("6...\n");
        sleep(1); printf("5...\n");
        sleep(1); printf("4...\n");
        sleep(1); printf("3...\n");
        sleep(1); printf("2...\n");
        sleep(1); printf("1...\n");
        printf("Capturing %d/%d...\n", i + 1, TOTAL_PICTURES);

        if (camera_capture_one_image(NFS_PATH, i) != 0) {
            fprintf(stderr, "Error capturing image %d\n", i);
        }
    }
    
    signal_host_complete(); // Tell python script we are done
    printf("Manual Scan Finished.\n");
    return 0;
}

// Waits for 'start_scan.txt' from Host, then runs motors + cam.
int run_auto_listening_mode(void) {
    printf("\n========================================\n");
    printf("   AUTO MODE ACTIVE - WAITING FOR HOST  \n");
    printf("   Watching: %s \n", CMD_FILE);
    printf("========================================\n");

    while (1) {
        if (access(CMD_FILE, F_OK) == 0) {
            printf("\nCommand received! detected %s\n", CMD_FILE);
            unlink(CMD_FILE); // Delete trigger file

            perform_scan_sequence();

            signal_host_complete();
            printf("Returning to idle state...\n");
        }
        sleep(1);
    }
    return 0;
}

// Helper logic for the Auto Mode
static int perform_scan_sequence(void) {
    int base_steps = STEPS_PER_ROTATION / TOTAL_PICTURES;
    int remainder = STEPS_PER_ROTATION % TOTAL_PICTURES;
    int steps_moved = 0;

    for (int i = 0; i < TOTAL_PICTURES; i++) {
        printf("Auto Capture %d/%d... ", i + 1, TOTAL_PICTURES);
        fflush(stdout);

        if (camera_capture_one_image(NFS_PATH, i) != 0) return -1;

        if (i < TOTAL_PICTURES - 1) {
            int steps = base_steps + (i < remainder ? 1 : 0);
            stepper_move_and_wait(steps);
            steps_moved += steps;
            usleep(500000); 
        }
    }

    // Complete rotation
    int remaining = STEPS_PER_ROTATION - steps_moved;
    if (remaining > 0) stepper_move_and_wait(remaining);
    
    return 0;
}

int main(void)
{
    if (camera_init() != 0)  { fprintf(stderr, "Camera init failed\n"); return 1; }
    
    server_init();
    htStepMotor_init();
    pfStepMotor_init();
    scanner_init();

    programRunning = true;
    
    while (programRunning){

    }

    // these functions are dead code now
    // this is from a previous implementation before the website was implemented
    // the thread initialization in scanner.c now handles this work and the main thread just sits empty
    // these functions won't work anymore

    // test_stepper_only();
    // test_camera_only();
    // run_manual_timed_sequence();
    // run_auto_listening_mode();

    scanner_cleanup();
    htStepMotor_cleanup();
    pfStepMotor_cleanup();
    camera_cleanup();
    server_cleanup();
    return 0;
}