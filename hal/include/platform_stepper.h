#ifndef _PLATFORM_STEPPER_H_
#define _PLATFORM_STEPPER_H_

// Initializes the GPIO lines and starts the platform stepper motor
void pfStepMotor_init(void);

// Rotates the platform based on pulses
void pfRotateStepper(int pulses, int direction);

// cleanup the platform stepper motor valuse GPIO lines.
void pfStepMotor_cleanup(void);

#endif