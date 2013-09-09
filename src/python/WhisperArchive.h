#ifndef _PY_WHISPER_ARCHIVE_INFO_H_
#define _PY_WHISPER_ARCHIVE_INFO_H_

#include <Python.h>
#include <structmember.h>

#include <wsp.h>

#include "WhisperException.h"

typedef struct {
    PyObject_HEAD;
    /* Type-specific fields go here. */
    /* stored reference of the database, since it is responsible for the
     * lifecycle of the wsp_archive_t object */
    PyObject *py_database;
    // archive index.
    uint32_t index;
    // seconds per point.
    uint32_t spp;
    // number of points.
    uint32_t count;
} WhisperArchive;

extern PyTypeObject WhisperArchive_T;

void init_WhisperArchive_T(PyObject *m);

PyObject *WhisperArchive_new(PyObject *py_database, uint32_t index);

#endif /* _PY_WHISPER_ARCHIVE_INFO_H_ */
