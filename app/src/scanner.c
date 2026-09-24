#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include "camera.h"
#include "scanner.h"
#include "platform_stepper.h"
#include "height_stepper.h"
#include "server.h"

extern bool programRunning;
bool isPaused;
bool isStoped;
bool startSignal;

static int mode;
static int customSamplePerRev;
static int customHeightChange;
static int customNumOfHeights;
static int focusHeight1 = 27;  
static int focusHeight2 = 20; 

static bool is_initialized = false;
static pthread_t scanner_thread_id;

int scanner_init(void){

    customSamplePerRev=0;
    customHeightChange=0;
    customNumOfHeights=0;
    mode=0;

    isPaused=false;
    isStoped=false;
    startSignal=false;

    if (pthread_create(&scanner_thread_id, NULL, scanner_thread_main, NULL) != 0) {
        perror("Failed to create scanner thread");
        return -1;
    }

    printf("Scanner App - Initializing\n");
    assert(!is_initialized);
    is_initialized = true;

    return 0;
}

int toggleIsPaused(){    
    //pervents pausing if stoped
    if(isStoped){isPaused = false;}
    else{isPaused =! isPaused;}
    return (int) isPaused;
}

void scanObject(int samplePerRev, int heightChange, int numOfHeights){

    int i;
    int i2;

    int base_pulses = PULSE_PER_REV / samplePerRev;
    int remainder = PULSE_PER_REV % samplePerRev;

    zeroCountHtPulses();
    
    // Give camera time to flush old frames before first capture
    printf("Flushing camera buffer...\n");
    usleep(1000000); // 1 second delay

    for(i=0; i<numOfHeights&&(!isStoped);i++){

        for(i2=0;i2<samplePerRev&&(!isStoped);i2++){
            if(!isStoped)camera_capture_one_image(NFS_PATH, i2+samplePerRev*i); 
            usleep(200000);
            pfRotateStepper(base_pulses, CLOCKWISE);
            usleep(200000);
        }

        pfRotateStepper(remainder+(samplePerRev-i2)*base_pulses, CLOCKWISE); //moves plater back to starting postion
        if(!isStoped&&numOfHeights-(i+1)){
            htRotateStepper(heightChange, CLOCKWISE);
            
            // Set focus for second height
            printf("Setting focus to %d for height level %d\n", focusHeight2, i+1);
            camera_set_focus(focusHeight2);
            
            printf("Please adjust camera angle and then press enter\n");
            int temp=0;
            scanf("%d",&temp);
        }
    }

    htSetToStart();
    if(!isStoped)signal_host_complete();

    return;
}

void* scanner_thread_main(void *arg){

    (void)arg;

    while(programRunning){
        if(startSignal){

            switch (mode)
            {
            case STANDARD_AUTO:
                scanObject(DEFAULT_SAMPLE_PER_REV,DEFAULT_HEIGHT_CHANGE,SD_NUMBER_OF_HEIGHTS);
                break;
            
            case DETAILED_AUTO:
                scanObject(DEFAULT_SAMPLE_PER_REV,DEFAULT_HEIGHT_CHANGE,DT_NUMBER_OF_HEIGHTS);
                break;
            
            case CUSTOM_MODE:
                if(customSamplePerRev!=0&&customHeightChange!=0&&customNumOfHeights!=0){
                    scanObject(customSamplePerRev,customHeightChange,customNumOfHeights);
                }else{
                    printf("Custom value where not set correctly: customSamplePerRev=%d, customHeightChange=%d, customNumOfHeights=%d\n",customSamplePerRev,customHeightChange,customNumOfHeights);
                }
                break;

            default:
                printf("Mode not set correctly \n");
                break;
            }

            isStoped = false;
            startSignal=false;
            printf("Returning to idle state...\n");
        }
    }
    printf("Program Termanating...\n");
    pthread_exit(0);
}

void signal_host_complete() {
    FILE *f = fopen(DONE_FILE, "w");
    if (f) { fprintf(f, "Scan Complete"); fclose(f); }
}

void scanner_cleanup(void){

    pthread_join(scanner_thread_id, NULL);

    printf("Scanner App - Cleanup\n");
    assert(is_initialized);
    is_initialized = false;

    return;
}

void manuallySetTestScan(void){

    int temp=0;

    printf("Set Mode: ");
    scanf("%d",&mode);
    printf("Set sample per rev: ");
    scanf("%d",&customSamplePerRev);
    printf("Set height change: ");
    scanf("%d",&customHeightChange);
    printf("Set number of heights: ");
    scanf("%d",&customNumOfHeights);
    printf("Set start signal: ");
    scanf("%d",&temp);
    setStartSignal((temp!=0));

    return;
}

void sendShutdownSignal(void){
    programRunning=false;
    isStoped=true;
    return;
}

void setCustomSamplePerRev(int Setvalue){
    if(Setvalue>0&&Setvalue<MAX_SAMPLE_PER_REV){customSamplePerRev=Setvalue;}
    else{
        customSamplePerRev=0;
        printf("SamplePerRev not set correctly \n");
    }
    return;
}

void setCustomHeightChange(int Setvalue){
    if(Setvalue>0&&Setvalue<MAX_HEIGHT_CHANGE){customHeightChange=Setvalue;}
    else{
        customHeightChange=0;
        printf("HeightChange not set correctly \n");
    }
    return;
}

void setCustomNumOfHeights(int Setvalue){
    if(Setvalue>0&&Setvalue<MAX_NUMBER_OF_HEIGHTS){customNumOfHeights=Setvalue;}
    else{
        customNumOfHeights=0;
        printf("NumOfHeights not set correctly \n");
    }
    return;
}

void setMode(int Setvalue){
    if(Setvalue>=0&&Setvalue<NUMBER_OF_MODES){mode=Setvalue;}
    else{
        mode=0;
        printf("mode not set correctly \n");
    }
    return;
}

void setFocusHeight1(int focus_value) {
    if (focus_value >= 0 && focus_value <= 250) {
        focusHeight1 = (focus_value / 5) * 5; // Round to step of 5
        printf("Focus height 1 set to: %d\n", focusHeight1);
    } else {
        printf("Focus value must be 0-250\n");
    }
}

void setFocusHeight2(int focus_value) {
    if (focus_value >= 0 && focus_value <= 250) {
        focusHeight2 = (focus_value / 5) * 5; // Round to step of 5
        printf("Focus height 2 set to: %d\n", focusHeight2);
    } else {
        printf("Focus value must be 0-250\n");
    }
}

void setIsStoped(bool setValue){
    isStoped=setValue;
    return;
}

void setStartSignal(bool setValue){
    startSignal=setValue;
    return;
}

void sendShutdownSignal(void){
    programRunning=false;
    isStoped=true;
    return;
}

int getCustomSamplePerRev(void){return customSamplePerRev;}
int getCustomHeightChange(void){return customHeightChange;}
int getCustomNumOfHeights(void){return customNumOfHeights;}
int getmode(void){return mode;}
