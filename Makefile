ARCHIVE=wsp.a

SOURCES+=src/wsp.c
SOURCES+=src/wsp_private.c
SOURCES+=src/wsp_time.c
SOURCES+=src/wsp_io_file.c
SOURCES+=src/wsp_io_mmap.c
SOURCES+=src/wsp_io_memory.c
SOURCES+=src/wsp_memfs.c

BINARIES+=src/whisper-dump
BINARIES+=src/whisper-create
BINARIES+=src/whisper-update

PREFIX:=/usr/local

WITH_DEBUG:="yes"
WITH_PYTHON:="yes"

CHECK_LIBS=$(shell pkg-config --libs check)

TESTS+=tests/test_wsp_create.test
TESTS+=tests/test_wsp_update.test

CFLAGS=-pedantic -Wall -std=c99 -fPIC -D_POSIX_C_SOURCE=200112

ifeq ($(WITH_DEBUG), "yes")
CFLAGS+=-g -DWSP_DEBUG
SOURCES+=src/wsp_debug.c
else
CFLAGS+=-O3
endif

OBJECTS+=$(SOURCES:.c=.o)

all: $(ARCHIVE) $(BINARIES) tests
	@if [[ $(WITH_PYTHON) != "yes" ]]; then\
		echo "Not building python bindings";\
    else\
	    make python-bindings;\
    fi

clean:
	$(RM) $(OBJECTS)
	$(RM) $(ARCHIVE)
	$(RM) $(BINARIES)
	$(RM) -R build

$(ARCHIVE): $(OBJECTS)
	$(AR) cr $@ $(OBJECTS)

%.test: %.o tests/check_utils.o $(ARCHIVE)
	$(CC) $< tests/check_utils.o $(CHECK_LIBS) $(ARCHIVE) -o $@

.PHONY: tests

tests: $(TESTS)
	@for test in $(TESTS); do echo "TEST: $$test"; $$test; done

.PHONY: python-bindings

python-bindings:
	python setup.py build

src/whisper-%: $(ARCHIVE)
	$(CC) $(CFLAGS) -o $@ $@.c $(ARCHIVE)
