/* stepper.c */
#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "stepper.h"

static pthread_t stepper_thread_id;
static pthread_mutex_t stepper_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  stepper_cond_start = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  stepper_cond_done  = PTHREAD_COND_INITIALIZER;

// These variables are shared between threads and protected by the mutex
static int  g_steps_to_move = 0;
static int  g_step_delay_us = 1000; // 1ms pulse = 500 steps/sec
static int  g_move_complete = 1;
static int  g_exit_thread = 0;

static struct gpiod_chip *chip;
static struct gpiod_line *step_line;
static struct gpiod_line *dir_line;

// This is the actual function that runs in its own thread
void* stepper_thread_main(void *arg) {
    (void)arg;
    while (1) {
        // Wait for a command
        pthread_mutex_lock(&stepper_mutex);
        while (!g_exit_thread && g_move_complete) {
            // Wait for 'stepper_move_and_wait' to give a command
            pthread_cond_wait(&stepper_cond_start, &stepper_mutex);
        }
        
        if (g_exit_thread) {
            pthread_mutex_unlock(&stepper_mutex);
            break; // Exit the loop
        }

        // We have a move command, so unlock mutex and do the work
        int steps = g_steps_to_move;
        pthread_mutex_unlock(&stepper_mutex);

        // --- Do the Hardware Work ---
        int dir = (steps > 0) ? 1 : 0;
        int step_count = (steps > 0) ? steps : -steps;
        
        gpiod_line_set_value(dir_line, dir);
        usleep(100); // Small delay for direction to set

        for (int i = 0; i < step_count; i++) {
            gpiod_line_set_value(step_line, 1);
            usleep(g_step_delay_us);
            gpiod_line_set_value(step_line, 0);
            usleep(g_step_delay_us);
        }
        // --- Work Done ---

        // Lock mutex to update state and signal 'main'
        pthread_mutex_lock(&stepper_mutex);
        g_move_complete = 1;
        pthread_cond_signal(&stepper_cond_done); // Signal 'main' that we are done
        pthread_mutex_unlock(&stepper_mutex);
    }
    return NULL;
}

int stepper_init(void) {
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) { perror("Error opening GPIO chip"); return -1; }

    step_line = gpiod_chip_get_line(chip, STEP_PIN);
    dir_line = gpiod_chip_get_line(chip, DIR_PIN);
    if (!step_line || !dir_line) {
        perror("Error getting GPIO lines");
        gpiod_chip_close(chip);
        return -1;
    }

    if (gpiod_line_request_output(step_line, "stepper", 0) < 0 ||
        gpiod_line_request_output(dir_line, "stepper", 0) < 0) {
        perror("Error requesting GPIO lines");
        gpiod_chip_close(chip);
        return -1;
    }

    // Start the thread
    if (pthread_create(&stepper_thread_id, NULL, stepper_thread_main, NULL) != 0) {
        perror("Failed to create stepper thread");
        return -1;
    }
    printf("Stepper thread initialized.\n");
    return 0;
}

void stepper_move_and_wait(int steps) {
    pthread_mutex_lock(&stepper_mutex);
    
    // Set the command
    g_steps_to_move = steps;
    g_move_complete = 0;
    
    // Wake up the stepper thread
    pthread_cond_signal(&stepper_cond_start);
    
    // Wait until the move is reported as complete
    while (!g_move_complete) {
        pthread_cond_wait(&stepper_cond_done, &stepper_mutex);
    }
    
    pthread_mutex_unlock(&stepper_mutex);
}

void stepper_cleanup(void) {
    // Tell the thread to exit
    pthread_mutex_lock(&stepper_mutex);
    g_exit_thread = 1;
    g_move_complete = 0; // Wake it up from its wait
    pthread_cond_signal(&stepper_cond_start);
    pthread_mutex_unlock(&stepper_mutex);
    
    // Wait for the thread to finish
    pthread_join(stepper_thread_id, NULL);

    // Clean up GPIO
    gpiod_line_release(step_line);
    gpiod_line_release(dir_line);
    gpiod_chip_close(chip);
    
    pthread_mutex_destroy(&stepper_mutex);
    pthread_cond_destroy(&stepper_cond_start);
    pthread_cond_destroy(&stepper_cond_done);
    printf("Stepper thread cleaned up.\n");
}