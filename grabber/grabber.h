#ifndef GRABBER_H
#define GRABBER_H

// This is the one function main.c needs to know about.
// It will start the picture-taking thread.
void grabber_take_pictures(void);

#endif // GRABBER_H