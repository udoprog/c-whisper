SOURCES+=src/wsp.c
SOURCES+=src/wsp_private.c
SOURCES+=src/wsp_time.c
SOURCES+=src/wsp_io_file.c
SOURCES+=src/wsp_io_mmap.c
OBJECTS=$(SOURCES:.c=.o)
ARCHIVE=wsp.a
PREFIX:=/usr/local

TESTS+=tests/test_wsp_io_file.1.test

CFLAGS=-g -pedantic -Wall -O3 -std=c99 -fPIC -D_POSIX_C_SOURCE

all: bin bin/whisper-dump python-bindings

bin:
	mkdir $@

clean:
	$(RM) $(OBJECTS)
	$(RM) $(ARCHIVE)
	$(RM) bin/*
	$(RM) -R build

%.test: %.o
	$(CC) $< $(shell pkg-config --libs check) -o $@

.PHONY: tests

tests: $(TESTS)
	@for test in $(TESTS); do echo "TEST: $$test"; $$test; done

.PHONY: python-bindings

python-bindings:
	python setup.py build

$(ARCHIVE): $(OBJECTS)
	$(AR) cr $@ $(OBJECTS)

bin/whisper-dump: src/whisper-dump.o $(ARCHIVE)
	$(CC) $(CFLAGS) -o $@ src/whisper-dump.o $(ARCHIVE)
