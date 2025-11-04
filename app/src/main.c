// This is the new content for app/src/main.c

#include <stdio.h>
#include <unistd.h>
#include "grabber.h" // The header file you will create (Step 3b in the guide)

int main(void) 
{
    printf("3D Scanner Application Started.\n");
    
    // This is the function you will create in grabber.c (Step 3d/3e in the guide)
    // It will start the thread that takes the pictures.
    grabber_take_pictures(); 
    
    printf("Picture-taking thread has been started.\n");
    
    // You can add other code here later, like for your stepper motors.
    // For now, we can just let the main thread sleep or exit.
    sleep(5); // Give the grabber thread 5 seconds to run

    printf("Main application exiting.\n");
    return 0;
}