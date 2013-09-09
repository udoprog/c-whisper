c-whisper
=========

A C implementation of the whisper database format.
https://github.com/graphite-project/whisper

**Disclaimer:**
I'm currently a noob at writing python extensions and might have gotten ref counting wrong on some occasions.
Feel free to proof check the code if you know more than me!

Introduction
============

I started prototyping this library with the 'whisper-dump' program found in the python library.
When comparing them head to head I often found that my naive C implementation performed far better in both utilizing the CPU and read/write performance.
That caused me to set out and write this, I wanted to maximize performance in my graphite installations to boost performance.

I am not attempting to be API compatible with the current python implementation since frankly; it's a mess.
Instead I am digging new ground and implementing a new API which supports the same functionality as the original, but it will require a refactor of the storage backend to be useful.
It also features a very clean C api, so writing your own applications outside of python should be possible.

Requirements
============

* A working C compiler (GCC 4.5/4.7, *probably* clang)
* (optional) Python development libraries for building extensions.

Building
========
Given that the stars are aligned and you have all required libraries available.
This will attempt to build everything including the python extensions.

  make clean all

To *avoid* building the python extension, invoke it like following;

  make WITH_PYTHON=no clean all

Someday I might switch build system, but make is incredible powerful done right.
