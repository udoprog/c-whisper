c-whisper
=========

A C implementation of the whisper database format.
https://github.com/graphite-project/whisper

**Disclaimer:**
I'm currently a beginner at pythons c-api and might have gotten ref counting
wrong.
If you know pythons c-api feel free to proof check the code under src/python.

Introduction
============

I started prototyping this library with the 'whisper-dump' program found in the 
python library.
When comparing them head to head I often found that my naive C implementation
performed far better in both utilizing the CPU and read/write performance.
That caused me to set out and write this. I want to maximize performance in
my graphite installation to scale a bit more vertically.

I am not attempting to be 100% API compatible with the current python
implementation since the complete api is a bit all over the place.
I do however aim at support the basic operations.

So far these include:

* *open* (wsp_open)
* *create* (wsp_create)
* *update* (wsp_update)
* *update\_many* (wsp_update_many)

It currently features a very clean C api, writing your own applications outside
of python is possible and encouraged.

Requirements
============

* A working C compiler (GCC 4.5/4.7, *probably* clang)
* (optional) Python development libraries for building extensions.

Building
========
Given that the stars are aligned and you have all required libraries available.
This will attempt to build everything including the python extensions.

    > make clean all

To *avoid* building the python extension, invoke it like following;

    > make WITH_PYTHON=no clean all

Make is incredible powerful, but someday the build system will probably change
to something more flexible.

LICENSE
=======
c-whisper is released under the LGPLv3, this gives you the following rights as
summarized from
[Wikipedia](http://en.wikipedia.org/wiki/GNU_Lesser_General_Public_License).

> The GNU Lesser General Public License or LGPL (formerly the GNU Library
> General Public License) is a free software license published by the Free
> Software Foundation (FSF). The LGPL allows developers and companies to use
> and integrate LGPL software into their own (even proprietary) software
> without being required (by the terms of a strong copyleft) to release the
> source code of their own software-parts. Merely the LGPL software-parts need
> to be modifiable by end-users (via source code availability): therefore, in
> the case of proprietary software, the LGPL-parts are usually used in the
> form of a shared library (e.g. DLL), so that there is a clear separation
> between the proprietary parts and open source LGPL parts.

Ask your legal team to look close at LICENSE, which should be a verbatim copy
of the plain text version found at
[gnu.org](http://www.gnu.org/licenses/lgpl-3.0.txt).

I ask (but do not require) for you to check back with me in any projects that
you decide to incorporate c-whisper into, mostly because I am curious.
