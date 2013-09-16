#include <Python.h>
#include <structmember.h>

#include "Whisper.h"
#include "WhisperArchive.h"
#include "WhisperMetadata.h"
#include "WhisperException.h"

#include <wsp.h>


static PyObject* _wsp_open(PyObject *self, PyObject *args) {
    PyObject *w = PyObject_CallObject((PyObject *)&Whisper_T, NULL);

    PyObject *w_open = PyObject_GetAttrString(w, "open");

    if (w_open == NULL) {
        return NULL;
    }

    if (PyObject_CallObject(w_open, args) == NULL) {
        return NULL;
    }

    return w;
}

static PyMethodDef py_wsp_methods[] = {
    {"open", _wsp_open, METH_VARARGS, "Open a whisper file"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initwsp(void) {
    PyObject *m = Py_InitModule("wsp", py_wsp_methods);

    PyModule_AddIntConstant(m, "MMAP", WSP_MMAP);
    PyModule_AddIntConstant(m, "FILE", WSP_FILE);
    PyModule_AddIntConstant(m, "MEMORY", WSP_MEMORY);

    PyModule_AddIntConstant(m, "AVERAGE", WSP_AVERAGE);
    PyModule_AddIntConstant(m, "SUM", WSP_SUM);
    PyModule_AddIntConstant(m, "LAST", WSP_LAST);
    PyModule_AddIntConstant(m, "MAX", WSP_MAX);
    PyModule_AddIntConstant(m, "MIN", WSP_MIN);

    init_WhisperException(m);

    init_Whisper_T(m);
    init_WhisperMetadata_T(m);
    init_WhisperArchive_T(m);
}
