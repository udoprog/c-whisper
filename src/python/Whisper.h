#ifndef _PY_WHISPER_H_
#define _PY_WHISPER_H_

#include <Python.h>
#include <structmember.h>

#include <wsp.h>

#include "WhisperMetadata.h"

typedef struct {
    PyObject_HEAD;
    /* Type-specific fields go here. */
    wsp_t *base;
    /* reference to metadata object */
    PyObject *py_meta;
    /* reference to a list of archive objects */
    PyObject *py_archives;
} Whisper;

extern PyTypeObject Whisper_T;

void init_Whisper_T(PyObject *m);

#endif /* _PY_WHISPER_H_ */
