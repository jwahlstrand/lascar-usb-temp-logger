#ifndef __LASCAR_H__
#define __LASCAR_H__ 1

#include <hidapi.h>

#define HUMIDITY 2
#define TEMPERATURE 3

void set_debug(int d);

/*
 * Retrieve temperature and humidity readings from the device
 */
int
get_reading(hid_device* hid,
            char* packet, float* temp, float* hum, int get_f);

/* the following functions are useful if only temp or humidity is desired */

/**
 * Return the temperature
 *
 * The second parameter can be 0 for Celcius or 1 for Farenheit
 */
float get_temp(unsigned int, int);

/**
 * Return the humidity as a percentage
 */
float get_hum(unsigned char);

/* the following functions are used internally and should not be needed */

/*
 * The retry parameter can be used to tell get_reading() to
 * automatically retry on error.
 */
int
get_reading_r(hid_device* hid,
              unsigned char* packet, float* temp, float* hum, int get_f, int retries);

/**
 * Read the specified number of bytes from the device
 */
int read_device(hid_device*, char*, int);

/**
 * Pack two chars into an int
 */
unsigned int pack(unsigned char, unsigned char);

#endif /* __LASCAR_H__ */
