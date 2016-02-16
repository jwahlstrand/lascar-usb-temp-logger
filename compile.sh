#!/bin/bash

gcc -c -g -mms-bitfields -IC:/msys32/usr/local/include/hidapi -IC:/msys32/usr/local/include/libsoup-2.4 -IC:/msys32/usr/local/include/glib-2.0 -IC:/msys32/usr/local/lib/glib-2.0/include -IC:/msys32/mingw32/include/libxml2 lascar.c -o lascar.o
gcc -c -g -mms-bitfields -IC:/msys32/usr/local/include/hidapi -IC:/msys32/usr/local/include/libsoup-2.4 -IC:/msys32/usr/local/include/glib-2.0 -IC:/msys32/usr/local/lib/glib-2.0/include -IC:/msys32/mingw32/include/libxml2 th_logger.c -o th_logger.o
gcc lascar.o th_logger.o -g -mms-bitfields -IC:/msys32/usr/local/include/hidapi -IC:/msys32/usr/local/include/libsoup-2.4 -IC:/msys32/usr/local/include/glib-2.0 -IC:/msys32/usr/local/lib/glib-2.0/include -IC:/msys32/mingw32/include/libxml2 -g -o th_logger -LC:/msys32/usr/local/lib -lhidapi -lsoup-2.4 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lintl 
