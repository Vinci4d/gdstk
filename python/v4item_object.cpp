/*
Copyright 2020 Lucas Heitzmann Gabrielli.
This file is part of gdstk, distributed under the terms of the
Boost Software License - Version 1.0.  See the accompanying
LICENSE file or <http://www.boost.org/LICENSE_1_0.txt>
*/

static PyObject* v4item_object_str(V4ItemObject* self) {
    char buffer[GDSTK_PRINT_BUFFER_COUNT];
    snprintf(buffer, COUNT(buffer), "V4Item '%s' at layer %" PRIu32 ", texttype %" PRIu32 "",
             self->v4item->text, get_layer(self->v4item->tag), get_type(self->v4item->tag));
    return PyUnicode_FromString(buffer);
}

static void v4item_object_dealloc(V4ItemObject* self) {
    if (self->v4item) {
        self->v4item->clear();
        free_allocation(self->v4item);
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static int v4item_object_init(V4ItemObject* self, PyObject* args, PyObject* kwds) {
    const char* text;
    PyObject* py_origin;
    PyObject* py_anchor = NULL;
    double rotation = 0;
    double magnification = 1;
    int x_reflection = 0;
    unsigned long layer = 0;
    unsigned long texttype = 0;
    const char* keywords[] = {"text",         "origin", "anchor",   "rotation", "magnification",
                              "x_reflection", "layer",  "texttype", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sO|Oddpkk:V4Item", (char**)keywords, &text,
                                     &py_origin, &py_anchor, &rotation, &magnification,
                                     &x_reflection, &layer, &texttype))
        return -1;

    if (self->v4item)
        self->v4item->clear();
    else
        self->v4item = (V4Item*)allocate_clear(sizeof(V4Item));

    V4Item* v4item = self->v4item;
    v4item->tag = make_tag(layer, texttype);
    v4item->text = copy_string(text, NULL);
    v4item->owner = self;
    return 0;
}

static PyObject* v4item_object_copy(V4ItemObject* self, PyObject*) {
    V4ItemObject* result = PyObject_New(V4ItemObject, &v4item_object_type);
    result = (V4ItemObject*)PyObject_Init((PyObject*)result, &v4item_object_type);
    result->v4item = (V4Item*)allocate_clear(sizeof(V4Item));
    result->v4item->copy_from(*self->v4item);
    result->v4item->owner = result;
    return (PyObject*)result;
}

static PyObject* v4item_object_deepcopy(V4ItemObject* self, PyObject* arg) {
    return v4item_object_copy(self, NULL);
}

static PyMethodDef v4item_object_methods[] = {
    {"copy", (PyCFunction)v4item_object_copy, METH_NOARGS, v4item_object_copy_doc},
    {"__deepcopy__", (PyCFunction)v4item_object_deepcopy, METH_VARARGS | METH_KEYWORDS, v4item_object_deepcopy_doc},
    {NULL}};

PyObject* v4item_object_get_text(V4ItemObject* self, void*) {
    PyObject* result = PyUnicode_FromString(self->v4item->text);
    if (!result) {
        PyErr_SetString(PyExc_TypeError, "Unable to convert value to string.");
        return NULL;
    }
    return result;
}

int v4item_object_set_text(V4ItemObject* self, PyObject* arg, void*) {
    if (!PyUnicode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "Text must be a string.");
        return -1;
    }

    Py_ssize_t len = 0;
    const char* src = PyUnicode_AsUTF8AndSize(arg, &len);
    if (!src) return -1;

    V4Item* v4item = self->v4item;
    v4item->text = (char*)reallocate(v4item->text, ++len);
    memcpy(v4item->text, src, len);
    return 0;
}

static PyObject* v4item_object_get_layer(V4ItemObject* self, void*) {
    return PyLong_FromUnsignedLongLong(get_layer(self->v4item->tag));
}

static int v4item_object_set_layer(V4ItemObject* self, PyObject* arg, void*) {
    set_layer(self->v4item->tag, (uint32_t)PyLong_AsUnsignedLongLong(arg));
    if (PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError, "Unable to convert layer to int.");
        return -1;
    }
    return 0;
}

static PyObject* v4item_object_get_texttype(V4ItemObject* self, void*) {
    return PyLong_FromUnsignedLongLong(get_type(self->v4item->tag));
}

static int v4item_object_set_texttype(V4ItemObject* self, PyObject* arg, void*) {
    set_type(self->v4item->tag, (uint32_t)PyLong_AsUnsignedLongLong(arg));
    if (PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError, "Unable to convert texttype to int.");
        return -1;
    }
    return 0;
}

static PyObject* v4item_object_get_arr0(V4ItemObject* self, void*) {
    const Array<double>* arr = &self->v4item->arr0;
    npy_intp dims[] = {(npy_intp)arr->count};
    PyObject* result = PyArray_SimpleNew(1, dims, NPY_DOUBLE);
    if (!result) {
        PyErr_SetString(PyExc_MemoryError, "Unable to create return array.");
        return NULL;
    }
    double* data = (double*)PyArray_DATA((PyArrayObject*)result);
    memcpy(data, arr->items, sizeof(double) * arr->count);
    return (PyObject*)result;
}

static PyObject* v4item_object_get_arr1(V4ItemObject* self, void*) {
    const Array<double>* arr = &self->v4item->arr0;
    npy_intp dims[] = {(npy_intp)arr->count};
    PyObject* result = PyArray_SimpleNew(1, dims, NPY_DOUBLE);
    if (!result) {
        PyErr_SetString(PyExc_MemoryError, "Unable to create return array.");
        return NULL;
    }
    double* data = (double*)PyArray_DATA((PyArrayObject*)result);
    memcpy(data, arr->items, sizeof(double) * arr->count);
    return (PyObject*)result;
}

static PyObject* v4item_object_get_layer_names(V4ItemObject* self, void*) {
    const std::vector<std::string> &arr = self->v4item->layer_names;
    PyObject* result = PyList_New(arr.size());
    if (!result) {
        PyErr_SetString(PyExc_MemoryError, "Unable to create return array.");
        return NULL;
    }
    for (uint64_t i = 0; i < arr.size(); i++) {
        PyList_SET_ITEM(result, i, PyUnicode_FromString(arr[i].c_str()));
    }
    return (PyObject*)result;
}

static PyObject* v4item_object_get_layer_numbers(V4ItemObject* self, void*) {
    const std::vector<int> &arr = self->v4item->layer_numbers;
    npy_intp dims[] = {(npy_intp)arr.size()};
    PyObject* result = PyArray_SimpleNew(1, dims, NPY_INT32);
    if (!result) {
        PyErr_SetString(PyExc_MemoryError, "Unable to create return array.");
        return NULL;
    }
    int* data = (int*)PyArray_DATA((PyArrayObject*)result);
    memcpy(data, arr.data(), sizeof(int) * arr.size());
    return (PyObject*)result;
}

static PyGetSetDef v4item_object_getset[] = {
    {"text",          (getter)v4item_object_get_text,           (setter)v4item_object_set_text,     v4item_object_text_doc,     NULL},
    {"arr0",          (getter)v4item_object_get_arr0,           NULL,                               NULL, NULL},
    {"arr1",          (getter)v4item_object_get_arr1,           NULL,                               NULL, NULL},
    {"layer",         (getter)v4item_object_get_layer,          (setter)v4item_object_set_layer,    v4item_object_layer_doc,    NULL},
    {"layer_names",   (getter)v4item_object_get_layer_names,    NULL,                               v4item_object_layer_doc,    NULL},
    {"layer_numbers", (getter)v4item_object_get_layer_numbers,  NULL,                               v4item_object_layer_doc,    NULL},
    {"texttype",      (getter)v4item_object_get_texttype,       (setter)v4item_object_set_texttype, v4item_object_texttype_doc, NULL},
    {NULL}
};
