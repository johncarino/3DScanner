#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <gpiod.h>
#include <stdbool.h>
#include "platform_stepper.h"
#include "scanner.h"

// very similar to the height_stepper implementation

#define PF_GPIO_PUL 9   //GPIO21
#define PF_GPIO_DIR 10  //GPIO20
#define PF_GPIO_ENA 7   //GPIO16
#define PF_CHIP_PATH "/dev/gpiochip2"

extern bool isPaused;

static double REV_PER_DEGREE;

static struct gpiod_chip *pfChip;
static struct gpiod_line *pfLinePul;
static struct gpiod_line *pfLineDir;
static struct gpiod_line *pfLineEna;

#ifndef SLEEPTIME
#define SLEEPTIME

static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}
#endif

static bool is_initialized = false;

void pfStepMotor_init(void){

    REV_PER_DEGREE=1;
    REV_PER_DEGREE=REV_PER_DEGREE/360;

    printf("Platform Stepper Motor - Initializing\n");
    assert(!is_initialized);
    is_initialized = true;

    pfChip = gpiod_chip_open(PF_CHIP_PATH);
    if (!pfChip) {
        perror("Open PF chip failed");
        return;
    }

    pfLinePul = gpiod_chip_get_line(pfChip, PF_GPIO_PUL);
    pfLineDir = gpiod_chip_get_line(pfChip, PF_GPIO_DIR);
    pfLineEna = gpiod_chip_get_line(pfChip, PF_GPIO_ENA);
    if (!pfLinePul||!pfLineDir||!pfLineEna ) {
        perror("Get PF lines failed");
        gpiod_chip_close(pfChip);
        return;
    }

    // Request the line as an output
    if (gpiod_line_request_output(pfLinePul, "Stepper_control", 0) < 0) {
        perror("Request PF_PUL line as output failed");
        gpiod_chip_close(pfChip);
        return;
    }

    if (gpiod_line_request_output(pfLineDir, "Stepper_control", 0) < 0) {
        perror("Request PF_DIR line as output failed");
        gpiod_chip_close(pfChip);
        return;
    }

    if (gpiod_line_request_output(pfLineEna, "Stepper_control", 0) < 0) {
        perror("Request PF_ENA line as output failed");
        gpiod_chip_close(pfChip);
        return;
    }

   return;
}

void pfRotateStepper(int pulses, int direction){

    printf("Platform Pulses: %d\n",pulses);

    gpiod_line_set_value(pfLineEna, true);
    gpiod_line_set_value(pfLineDir, direction);

    for(int i=0; i<pulses; i++){

        if(isPaused){
            i--;
        }else{
            gpiod_line_set_value(pfLinePul, true);
            sleepForMs(1);
            gpiod_line_set_value(pfLinePul, false);
            sleepForMs(1);
        }
    }
    gpiod_line_set_value(pfLineEna, false);
    gpiod_line_set_value(pfLineDir, false);

    return;
}

void pfStepMotor_cleanup(void){

    // free any memory, close files, ...
    printf("Platform Stepper Motor - Cleanup\n");
    assert(is_initialized);
    is_initialized = false;

    gpiod_line_set_value(pfLinePul, 0);
    gpiod_line_set_value(pfLineDir, 0);
    gpiod_line_set_value(pfLineEna, 0);

    gpiod_line_release(pfLinePul);
    gpiod_line_release(pfLineDir);
    gpiod_line_release(pfLineEna);
    gpiod_chip_close(pfChip);
    return;
}