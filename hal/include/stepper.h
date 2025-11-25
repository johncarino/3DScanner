/* stepper.h */
#ifndef STEPPER_H
#define STEPPER_H

// CONFIGURATION
// !! CHANGE THIS to the chip you found with 'gpiodetect'
#define GPIO_CHIP "gpiochip0" 
#define STEP_PIN 7 //23
#define DIR_PIN  10 //24

/* 
 * Initializes the GPIO lines and starts the stepper motor thread.
 * The thread will wait for commands.
 * Returns 0 on success, -1 on failure.
 */
int stepper_init(void);

/*
 * Commands the stepper thread to move a number of steps.
 * This function will WAIT until the move is complete before returning.
 * steps: Number of steps to move. Positive for one dir, negative for another.
 */
void stepper_move_and_wait(int steps);

/*
 * Stops the stepper thread and cleans up GPIO.
 */
void stepper_cleanup(void);

#endif // STEPPER_H