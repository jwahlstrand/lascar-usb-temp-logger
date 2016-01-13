#include <stdio.h>
#include "lascar.h"

float get_temp(unsigned int t, int get_f) {
    float rt = -200.0;

    for(; t>0; t--) {
        rt += STEPT;
    }

    if(get_f) {
      rt = (rt * (9.0/5.0)) + 32;
    }

    return rt;
}

float get_hum(unsigned char h) {
    float rh = 0;

    for(; h>0x00; h--) {
        rh+= STEPH;
    }

    return rh;
}

int
get_reading(hid_device* hid, char* packet,
              float* temp, float* hum, int get_f) {
    return get_reading_r(hid, packet, temp, hum, get_f, 1);
}

int
get_reading_r(hid_device* hid, char* packet,
              float* temp, float* hum, int get_f, int retries) {
    int ret;

    /*
     * The temperature and humidity values are always sent one after the other,
     * i.e. there is no way to request the same value back to back; you're
     * going to get temp, hum, temp, hum, [...].  We know simply know if the
     * data retrieved has 3 bytes it's temperature and if it has two bytes it's
     * humidity.
     *
     * If we grab the temperature first and it fails, we can assume the device
     * sent us humidity, so throw it out and re-request the data.  If for some
     * reason the subsequent request fails, then there is a problem and return
     * an error.
     */
    if((ret=read_device(hid, packet, TEMPERATURE)) != 3) {
        if(ret == 2 && retries) { /* we got a humidity reading */
            fprintf(stderr, "Got %d bytes, retrying\n",ret);
            return get_reading_r(hid, packet, temp, hum, get_f, --retries);
        } else {
            fprintf(stderr, "Unable to read temperature (%d)\n", TEMPERATURE);
            return ret;
        }
    }

    *temp = get_temp(pack((unsigned)packet[2], (unsigned)packet[1]), get_f);

    if((ret=read_device(hid, packet, HUMIDITY)) != 2) {
        fprintf(stderr, "Unable to read humidity (%d)\n", HUMIDITY);
        return ret;
    }

    *hum = get_hum((unsigned)packet[1]);

    /* check to make sure the values found are within spec, otherwise retry */
    if(*temp >= -200.0 && *temp <= 200.0 && *hum >= 0.0 && *hum <= 100.0) {
        return ret;
    } else if(retries) {
        /* if the values were bad, try another two times before giving up */
        /*fprintf(stderr,
                  "Bad values for temp (%.1f) and hum (%.1f)\n", temp, hum);*/
        return get_reading_r(hid, packet, temp, hum, get_f, 2);
    } else {
        return -2;
    }
}

int read_device(hid_device* hid, char* buf, int size) {
    int i;

    i = hid_read_timeout(hid, buf, size, 520);

    if(i < 0) {
        fprintf(stderr, "hid_get_input_report failed with return code %d\n", i);
    }

    return i;
}

unsigned int pack(unsigned char a, unsigned char b) {
    unsigned int packed;

    packed = a;
    packed <<= 8;
    packed |= b;

    return packed;
}
