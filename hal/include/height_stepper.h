#ifndef _HEIGHT_STEPPER_H_
#define _HEIGHT_STEPPER_H_


// initializes the GPIO lines and starts the height stepper motor
void htStepMotor_init(void);

// changes the height of the camera 
void htRotateStepper(int heightChange, int direction);

// zeros the CountHtPulses value to 0
void zeroCountHtPulses(void);

// sets the height stepper motor back to it's starting height
void htSetToStart(void);

// cleanup the height stepper motor valuse GPIO lines.
void htStepMotor_cleanup(void);

#endif