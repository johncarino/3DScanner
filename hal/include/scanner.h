#ifndef SCANNER_H
#define SCANNER_H

// scan Modes
#define STANDARD_AUTO 0
#define DETAILED_AUTO 1
#define CUSTOM_MODE 2

// default settings
#define DEFAULT_SAMPLE_PER_REV 20
#define DEFAULT_HEIGHT_CHANGE 30
#define SD_NUMBER_OF_HEIGHTS 1
#define DT_NUMBER_OF_HEIGHTS 2

// stepper motor definitons
#define CLOCKWISE 0
#define COUNTER_CLOCKWISE 1
#define PULSE_PER_REV 3200
#define HT_PULSE_PER_REV 200

// Max values
#define MAX_SAMPLE_PER_REV 32
#define MAX_HEIGHT_CHANGE 50
#define MAX_NUMBER_OF_HEIGHTS 3
#define NUMBER_OF_MODES 3

// paths
#define NFS_PATH "/mnt/nfs_share/myApps"
#define DONE_FILE "/mnt/nfs_share/myApps/done.txt"
#define CMD_FILE "/mnt/nfs_share/myApps/start_scan.txt"

int scanner_init(void);
int toggleIsPaused();
void scanObject(int samplePerRev, int heightChange, int numOfHeights);
void* scanner_thread_main(void *arg);
void signal_host_complete();
void scanner_cleanup(void);

void manuallySetTestScan(void);
void sendShutdownSignal(void);

// setters and getters
void setCustomSamplePerRev(int Setvalue);
void setCustomHeightChange(int Setvalue);
void setCustomNumOfHeights(int Setvalue);
void setMode(int Setvalue);
void setFocusHeight1(int focus_value);
void setFocusHeight2(int focus_value);
void setIsStoped(bool setValue);
void setStartSignal(bool setValue);

int getCustomSamplePerRev(void);
int getCustomHeightChange(void);
int getCustomNumOfHeights(void);
int getmode(void);

#endif