c-whisper
=========

A C implementation of the whisper database format.
https://github.com/graphite-project/whisper

*Disclaimer*
I currently a noob at writing python extensions and might have gotten ref counting wrong on some occasions.
Feel free to proof check the code if you know more than me!

Introduction
============

I started prototyping this library, or specifically the 'whisper-dump' functionality of the python implementation.
When comparing them head to head I often found that my naive C implementation performed far better in both utilizing the CPU and in read/write performance.
That caused me to set out and write this, I wanted to use this in one of my graphite installations to boost performance.

I am not attempting to be API compatible with the current python implementation since frankly it's a mess.
Instead I am digging new ground and implementing a new API which supports the same functionality as the old one.

Requirements
============

* A working C compiler (GCC 4.5/4.7, probably clang)
* (optional) Python development libraries for building extensions.

Building
========
Given that the stars are aligned and you have all required libraries available.
This will attempt to build everything including the python extensions.

  make clean all
