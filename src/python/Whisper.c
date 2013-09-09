#include "Whisper.h"
#include "WhisperArchive.h"

typedef Whisper C;

static PyObject* Whisper_open(C *self, PyObject *args) {
    char *path;
    wsp_t *base = NULL;
    PyObject *py_meta = NULL;
    PyObject *py_archives = NULL;
    wsp_mapping_t mapping = WSP_MAPPING_NONE;

    if (!PyArg_ParseTuple(args, "s|i", &path, &mapping)) {
        return NULL;
    }

    if (mapping == WSP_MAPPING_NONE) {
        mapping = WSP_MMAP;
    }

    if (self->py_meta != Py_None) {
        PyObject *tmp = self->py_meta;
        self->py_meta = Py_None;
        Py_DECREF(tmp);
        Py_INCREF(self->py_meta);
    }

    if (self->py_archives != Py_None) {
        PyObject *tmp = self->py_archives;
        self->py_archives = Py_None;
        Py_DECREF(tmp);
        Py_INCREF(self->py_archives);
    }

    base = PyMem_Malloc(sizeof(wsp_t));

    if (base == NULL) {
        return NULL;
    }

    WSP_INIT(base);

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    if (wsp_open(base, path, mapping, WSP_READ | WSP_WRITE, &e) == WSP_ERROR) {
        PyMem_Free(base);
        PyErr_Whisper(&e);
        return NULL;
    }

    self->base = base;

    py_meta = WhisperMetadata_new((PyObject *)self);

    if (py_meta == NULL) {
        goto error;
    }

    py_archives = PyList_New(0);

    if (py_archives == NULL) {
        goto error;
    }

    // build archive objects.
    uint32_t i = 0;
    for (i = 0; i < self->base->archives_count; i++) {
        PyObject *archive = WhisperArchive_new((PyObject *)self, i);

        if (archive == NULL || PyList_Append(py_archives, archive) == -1) {
            Py_XDECREF(archive);
            goto error;
        }

        Py_DECREF(archive);
    }

    PyObject *tmp;

    tmp = self->py_meta;
    self->py_meta = py_meta;
    Py_INCREF(self->py_meta);
    Py_DECREF(tmp);

    tmp = self->py_archives;
    self->py_archives = py_archives;
    Py_INCREF(self->py_archives);
    Py_DECREF(tmp);

    Py_RETURN_NONE;

error:
    if (base != NULL) {
        wsp_close(base, NULL);
        PyMem_Free(base);
    }

    Py_XDECREF(py_meta);
    Py_XDECREF(py_archives);
    return NULL;
}

static PyObject* Whisper_load_points(C *self, PyObject *args) {
    PyObject *p_archive;

    if (!PyArg_ParseTuple(args, "O", &p_archive)) {
        return NULL;
    }

    switch (PyObject_IsInstance(p_archive, (PyObject *)&WhisperArchive_T)) {
        case -1:
            return NULL;
        case 0:
            PyErr_SetString(PyExc_TypeError, "Expected type 'WhisperArchive'");
            return NULL;
        default:
            break;
    }

    WhisperArchive *py_archive = (WhisperArchive *)p_archive;

    PyObject *result = PyList_New(0);

    if (result == NULL) {
        return NULL;
    }

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    wsp_archive_t *archive = &self->base->archives[py_archive->index];
    wsp_point_t points[archive->count];

    if (wsp_load_points(self->base, archive, 0, archive->count, points, &e) == WSP_ERROR) {
        PyErr_Whisper(&e);
        Py_DECREF(result);
        return NULL;
    }

    uint32_t i;
    wsp_point_t point;

    for (i = 0; i < archive->count; i++) {
        point = points[i];

        PyObject *tuple = Py_BuildValue("(Ld)", point.timestamp, point.value);

        if (tuple == NULL || PyList_Append(result, tuple) == -1) {
            Py_XDECREF(tuple);
            Py_DECREF(result);
            return NULL;
        }

        // tuple is now owner by result list.
        Py_DECREF(tuple);
    }

    Py_INCREF(result);
    return result;
}

static PyObject* Whisper_update_point(C *self, PyObject *args) {
    unsigned int i_timestamp;
    double value;

    if (!PyArg_ParseTuple(args, "Id", &i_timestamp, &value)) {
        return NULL;
    }

    uint32_t timestamp = (uint32_t)i_timestamp;

    if (self->base == NULL) {
        PyErr_SetString(PyExc_Exception, "Base not initialized");
        return NULL;
    }

    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    wsp_point_t p = {
        .timestamp = timestamp,
        .value = value
    };

    if (wsp_update(self->base, &p, &e) == WSP_ERROR) {
        PyErr_Whisper(&e);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef Whisper_methods[] = {
    {"open", (PyCFunction)Whisper_open, METH_VARARGS, "Open the specified path"},
    {"load_points", (PyCFunction)Whisper_load_points, METH_VARARGS, "Load points"},
    {"update_point", (PyCFunction)Whisper_update_point, METH_VARARGS, "Update point"},
    {NULL}
};

static PyMemberDef Whisper_members[] = {
    {"archives", T_OBJECT_EX, offsetof(Whisper, py_archives), 0, "Archives"},
    {"meta", T_OBJECT_EX, offsetof(Whisper, py_meta), 0, "Metadata"},
    {NULL}
};

static int
Whisper_init(C *self, PyObject *args, PyObject *kwds) {
    self->base = NULL;

    self->py_meta = Py_None;
    Py_INCREF(self->py_meta);

    self->py_archives = Py_None;
    Py_INCREF(self->py_archives);

    return 0;
}

static void
Whisper_dealloc(C *self) {
    Py_XDECREF(self->py_meta);
    Py_XDECREF(self->py_archives);
    printf("Dealloc\n");
}

PyTypeObject Whisper_T = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "Whisper", /*tp_name*/
    sizeof(C), /*tp_basicsize*/
    0, /*tp_itemsize*/
    (destructor)Whisper_dealloc, /*tp_dealloc*/
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
    "Whisper", /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    Whisper_methods, /*tp_methods*/
    Whisper_members, /*tp_members*/
    0, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    (initproc)Whisper_init, /*tp_init*/
    0, /*tp_alloc*/
    0, /*tp_new*/
};

void init_Whisper_T(PyObject *m) {
    Whisper_T.tp_new = PyType_GenericNew;

    if (PyType_Ready(&Whisper_T) == 0) {
        Py_INCREF(&Whisper_T);
        PyModule_AddObject(m, "Whisper", (PyObject *)&Whisper_T);
    }
}
