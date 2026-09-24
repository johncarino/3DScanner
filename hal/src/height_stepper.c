#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <gpiod.h>
#include <stdbool.h>
#include "height_stepper.h"
#include "scanner.h"

#define REV_PER_MM 0.5

#define HT_GPIO_PUL 2 //GPIO11
#define HT_GPIO_DIR 4 //GPIO9
#define HT_GPIO_ENA 3 //GPIO10
#define HT_CHIP_PATH "/dev/gpiochip0"

extern bool isPaused;
extern bool isStoped;

static int countHtPulses;

static struct gpiod_chip *htChip;
static struct gpiod_line *htLinePul;
static struct gpiod_line *htLineDir;
static struct gpiod_line *htLineEna;

#ifndef SLEEPTIME
#define SLEEPTIME

static void sleepForMs(long long delayInMs){
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;

    long long delayNs = delayInMs * NS_PER_MS;
    
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

#endif

// Allow module to ensure it has been initialized (once!)
static bool is_initialized = false;

void htStepMotor_init(void){

    printf("Height Stepper Motor - Initializing\n");
    assert(!is_initialized);
    is_initialized = true;

    // Open the GPIO chip
    htChip = gpiod_chip_open(HT_CHIP_PATH);
    
    if (!htChip) {
        perror("Open HTchip failed");
        return;
    }

    // Get the specific line
    htLinePul = gpiod_chip_get_line(htChip, HT_GPIO_PUL);
    htLineDir = gpiod_chip_get_line(htChip, HT_GPIO_DIR);
    htLineEna = gpiod_chip_get_line(htChip, HT_GPIO_ENA);
    
    if (!htLinePul||!htLineDir||!htLineEna ) {
        perror("Get HTlines failed");
        gpiod_chip_close(htChip);
        return;
    }
    // Request the line as an output
    if (gpiod_line_request_output(htLinePul, "Stepper_control", 0) < 0) {
        perror("Request HT_PUL line as output failed");
        gpiod_chip_close(htChip);
        return;
    }

    if (gpiod_line_request_output(htLineDir, "Stepper_control", 0) < 0) {
        perror("Request HT_DIR line as output failed");
        gpiod_chip_close(htChip);
        return;
    }

    if (gpiod_line_request_output(htLineEna, "Stepper_control", 0) < 0) {
        perror("Request HT_ENA line as output failed");
        gpiod_chip_close(htChip);
        return;
    }

   return;
}

void htRotateStepper(int heightChange, int direction){

    int pulses = HT_PULSE_PER_REV*REV_PER_MM * heightChange;

    printf("Height Pulses: %d\n",pulses);

    gpiod_line_set_value(htLineEna, true);
    gpiod_line_set_value(htLineDir, direction);

    for(int i=0; i<pulses&&!isStoped; i++){

        if(isPaused){
            i--;
        }
        else{
            gpiod_line_set_value(htLinePul, true);
            sleepForMs(1);
            
            gpiod_line_set_value(htLinePul, false);
            sleepForMs(1);
            countHtPulses++;
        }
    }

    gpiod_line_set_value(htLineEna, false);
    gpiod_line_set_value(htLineDir, false);
    return;
}

void zeroCountHtPulses(void){
    countHtPulses=0;
    return;
}

void htSetToStart(void){

    printf("Height Pulses: %d\n",countHtPulses);

    gpiod_line_set_value(htLineEna, true);
    gpiod_line_set_value(htLineDir, COUNTER_CLOCKWISE);

    for(int i=0; i<countHtPulses; i++){
        gpiod_line_set_value(htLinePul, true);
        sleepForMs(1);

        gpiod_line_set_value(htLinePul, false);
        sleepForMs(1);
    }

    gpiod_line_set_value(htLineEna, false);
    gpiod_line_set_value(htLineDir, false);
    return;
}

void htStepMotor_cleanup(void){
    // free any memory, close files, ...
    printf("Height Stepper Motor - Cleanup\n");
    assert(is_initialized);
    is_initialized = false;

    gpiod_line_set_value(htLinePul, 0);
    gpiod_line_set_value(htLineDir, 0);
    gpiod_line_set_value(htLineEna, 0);

    gpiod_line_release(htLinePul);
    gpiod_line_release(htLineDir);
    gpiod_line_release(htLineEna);
    gpiod_chip_close(htChip);
    return;
}