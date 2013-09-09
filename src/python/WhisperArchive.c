#include "WhisperArchive.h"

#include <wsp.h>

#include "Whisper.h"

typedef WhisperArchive C;

/*
 * Update stored values for this archive from the database references.
 */
void WhisperArchive__reload(C *self)
{
    Whisper *database = (Whisper *)self->py_database;
    wsp_archive_t *archive = &(database->base->archives[self->index]);

    self->count = archive->count;
    self->spp = archive->spp;
}

static PyMethodDef WhisperArchive_methods[] = {
    {NULL}
};

static PyMemberDef WhisperArchive_members[] = {
    {"db", T_OBJECT_EX, offsetof(WhisperArchive, py_database), 0, "Owner Database"},
    {"spp", T_INT, offsetof(WhisperArchive, spp), 0, "Seconds Per Point"},
    {"count", T_INT, offsetof(WhisperArchive, count), 0, "Points Count"},
    {NULL}
};

static int
WhisperArchive_init(C *self, PyObject *args, PyObject *kwds) {
    PyObject *py_database = NULL;
    uint32_t index = 0;

    if (!PyArg_ParseTuple(args, "Oi", &py_database, &index)) {
        return -1; 
    }

    switch (PyObject_IsInstance(py_database, (PyObject *)&Whisper_T)) {
        case -1:
            return -1;
        case 0:
            PyErr_SetString(PyExc_TypeError, "Expected 'Whisper' type argument");
            return -1;
        default:
            break;
    }

    self->py_database = py_database;
    Py_INCREF(self->py_database);
    self->index = index;

    WhisperArchive__reload(self);

    return 0;
}

static void
WhisperArchive_dealloc(C *self) {
    Py_DECREF(self->py_database);
}

PyTypeObject WhisperArchive_T = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "WhisperArchive", /*tp_name*/
    sizeof(C), /*tp_basicsize*/
    0, /*tp_itemsize*/
    (destructor)WhisperArchive_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    0, /*tp_getattr*/
    0, /*tp_setattr*/
    0, /*tp_compare*/
    0, /*tp_repr*/
    0, /*tp_as_number*/
    0, /*tp_as_sequence*/
    0, /*tp_as_mapping*/
    0, /*tp_hash */
    0, /*tp_call*/
    0, /*tp_str*/
    0, /*tp_getattro*/
    0, /*tp_setattro*/
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT, /*tp_flags*/
    "Whisper Archive", /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    WhisperArchive_methods, /*tp_methods*/
    WhisperArchive_members, /*tp_members*/
    0, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    (initproc)WhisperArchive_init, /*tp_init*/
    0, /*tp_alloc*/
    0, /*tp_new*/
};

PyObject *WhisperArchive_new(PyObject *file, uint32_t index)
{
    PyObject *args = Py_BuildValue("Oi", file, index);

    if (args == NULL) {
        return NULL;
    }

    Py_INCREF(args);
    return PyObject_CallObject((PyObject *)&WhisperArchive_T, args);
}

void init_WhisperArchive_T(PyObject *m) {
    WhisperArchive_T.tp_new = PyType_GenericNew;

    if (PyType_Ready(&WhisperArchive_T) == 0) {
        Py_INCREF(&WhisperArchive_T);
        PyModule_AddObject(m, "WhisperArchive", (PyObject *)&WhisperArchive_T);
    }
}
