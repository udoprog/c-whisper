#ifndef _PY_WHISPER_EXCEPTION_H_
#define _PY_WHISPER_EXCEPTION_H_

#include <Python.h>
#include <wsp.h>

extern PyObject *WhisperException;

void init_WhisperException(PyObject *m);
void PyErr_Whisper(wsp_error_t *e);

#endif /* _PY_WHISPER_EXCEPTION_H_ */
