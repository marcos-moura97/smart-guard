#!/bin/bash

cc ./misc/bootselBoot.c -o ./misc/bootselBoot -I/usr/include/libusb-1.0 -L/usr/lib -lusb-1.0
./misc/bootselBoot
