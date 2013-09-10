SOURCES+=src/wsp.c
SOURCES+=src/wsp_private.c
SOURCES+=src/wsp_time.c
SOURCES+=src/wsp_io_file.c
SOURCES+=src/wsp_io_mmap.c
ARCHIVE=wsp.a
PREFIX:=/usr/local

WITH_DEBUG:="yes"
WITH_PYTHON:="yes"

CHECK_LIBS=$(shell pkg-config --libs check)

TESTS+=tests/test_wsp_io_file.1.test

CFLAGS=-pedantic -Wall -std=c99 -fPIC -D_POSIX_C_SOURCE=200112

ifeq ($(WITH_DEBUG), "yes")
CFLAGS+=-g -DWSP_DEBUG
SOURCES+=src/wsp_debug.c
else
CFLAGS+=-O3
endif

OBJECTS=$(SOURCES:.c=.o)

all: bin/whisper-dump
	@if [[ $(WITH_PYTHON) != "yes" ]]; then\
		echo "Not building python bindings";\
    else\
	    make python-bindings;\
    fi

clean:
	$(RM) $(OBJECTS)
	$(RM) $(ARCHIVE)
	$(RM) bin/*
	$(RM) -R build

%.test: %.o
	$(CC) $< $(CHECK_LIBS) -o $@

.PHONY: tests

tests: $(TESTS)
	@for test in $(TESTS); do echo "TEST: $$test"; $$test; done

.PHONY: python-bindings

python-bindings:
	python setup.py build

$(ARCHIVE): $(OBJECTS)
	$(AR) cr $@ $(OBJECTS)

bin:
	mkdir $@

bin/whisper-dump: bin src/whisper-dump.o $(ARCHIVE)
	$(CC) $(CFLAGS) -o $@ src/whisper-dump.o $(ARCHIVE)
