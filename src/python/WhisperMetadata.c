#include "WhisperMetadata.h"

typedef WhisperMetadata C;

void WhisperMetadata__reload(C *self)
{
    Whisper *database = (Whisper *)self->py_database;
    wsp_metadata_t *metadata = &(database->base->meta);

    self->aggregation = metadata->aggregation;
    self->max_retention = metadata->max_retention;
    self->x_files_factor = metadata->x_files_factor;
    self->archives_count = metadata->archives_count;
}

static PyMethodDef WhisperMetadata_methods[] = {
    {NULL}
};

static PyMemberDef WhisperMetadata_members[] = {
    {"db", T_OBJECT_EX, offsetof(WhisperMetadata, py_database), 0, "Owner database"},
    {"aggregation", T_INT, offsetof(WhisperMetadata, aggregation), 0, "Aggregation Type"},
    {"max_retention", T_INT, offsetof(WhisperMetadata, max_retention), 0, "Max Retention"},
    {"x_files_factor", T_FLOAT, offsetof(WhisperMetadata, x_files_factor), 0, "X Files Factor"},
    {"archives_count", T_INT, offsetof(WhisperMetadata, archives_count), 0, "Archives Count"},
    {NULL}
};

static int
WhisperMetadata_init(C *self, PyObject *args, PyObject *kwds)
{
    PyObject *p_w = NULL;

    if (!PyArg_ParseTuple(args, "O", &p_w)) {
        return -1; 
    }

    switch (PyObject_IsInstance(p_w, (PyObject *)&Whisper_T)) {
        case -1:
            return -1;
        case 0:
            PyErr_SetString(PyExc_TypeError, "Expected 'Whisper' type argument");
            return -1;
        default:
            break;
    }

    self->py_database = p_w;
    Py_INCREF(self->py_database);

    WhisperMetadata__reload(self);

    return 0;
}

PyTypeObject WhisperMetadata_T = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "WhisperMetadata", /*tp_name*/
    sizeof(C), /*tp_basicsize*/
    0, /*tp_itemsize*/
    0, /*tp_dealloc*/
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
    "Whisper File", /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    WhisperMetadata_methods, /*tp_methods*/
    WhisperMetadata_members, /*tp_members*/
    0, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    (initproc)WhisperMetadata_init, /*tp_init*/
    0, /*tp_alloc*/
    0, /*tp_new*/
};

PyObject *WhisperMetadata_new(PyObject *py_database)
{
    PyObject *args = Py_BuildValue("(O)", py_database);

    if (args == NULL) {
        return args;
    }

    Py_INCREF(args);

    PyObject *self = PyObject_CallObject((PyObject *)&WhisperMetadata_T, args);

    return self;
}

void init_WhisperMetadata_T(PyObject *m) {
    WhisperMetadata_T.tp_new = PyType_GenericNew;

    if (PyType_Ready(&WhisperMetadata_T) == 0) {
        Py_INCREF(&WhisperMetadata_T);
        PyModule_AddObject(m, "WhisperMetadata", (PyObject *)&WhisperMetadata_T);
    }
}
