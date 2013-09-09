#ifndef _PY_WHISPER_METADATA_H_
#define _PY_WHISPER_METADATA_H_

#include <Python.h>
#include <structmember.h>

#include <wsp.h>

#include "WhisperException.h"
#include "Whisper.h"

typedef struct {
    PyObject_HEAD;
    /* Type-specific fields go here. */
    PyObject *py_database;
    uint32_t aggregation;
    uint32_t max_retention;
    float x_files_factor;
    uint32_t archives_count;
} WhisperMetadata;

extern PyTypeObject WhisperMetadata_T;

void init_WhisperMetadata_T(PyObject *m);

PyObject *WhisperMetadata_new(PyObject *py_database);

#endif /* _PY_WHISPER_METADATA_H_ */
