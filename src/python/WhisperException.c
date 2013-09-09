#include "WhisperException.h"

PyObject *WhisperException;

void PyErr_Whisper(wsp_error_t *e) {
    if (e->syserr != 0) {
        PyErr_Format(
            WhisperException,
            "%s: %s",
            wsp_strerror(e),
            strerror(e->syserr));
        return;
    }

    PyErr_Format(
        WhisperException,
        "%s",
        wsp_strerror(e));
}

void init_WhisperException(PyObject *m) {
    WhisperException = PyErr_NewException("wsp.WhisperException", NULL, NULL);
    Py_INCREF(WhisperException);
    PyModule_AddObject(m, "WhisperException", WhisperException);
}
