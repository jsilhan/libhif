#include <Python.h>
#include <stdio.h>

// libsolv
#include <util.h>

// pyhawkey
#include "iutil-py.h"
#include "package-py.h"
#include "sack-py.h"

typedef struct {
    PyObject_HEAD
    Package package;
    PyObject *sack;
} _PackageObject;

Package packageFromPyObject(PyObject *o)
{
    if (!packageObject_Check(o)) {
	PyErr_SetString(PyExc_TypeError, "Expected a Package object.");
	return NULL;
    }
    return ((_PackageObject *)o)->package;
}

PyObject *new_package(PyObject *sack, Package cpkg)
{
    PyObject *package, *arglist;

    arglist = Py_BuildValue("Oi", sack, cpkg->id);
    if (arglist == NULL)
	return NULL;
    package = PyObject_CallObject((PyObject*)&package_Type, arglist);
    Py_DECREF(arglist);

    return package;
}

/* functions on the type */

static PyObject *
package_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _PackageObject *self = (_PackageObject*)type->tp_alloc(type, 0);
    if (self) {
	self->sack = NULL;
	self->package = NULL;
    }
    return (PyObject*)self;
}

static void
package_dealloc(_PackageObject *self)
{
    if (self->package)
	package_free(self->package);

    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static int
package_init(_PackageObject *self, PyObject *args, PyObject *kwds)
{
    Id id;
    PyObject *sack;
    Sack csack;

    if (!PyArg_ParseTuple(args, "O!i", &sack_Type, &sack, &id))
	return -1;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
	return -1;
    self->sack = sack;
    Py_INCREF(self->sack);
    self->package = package_create(csack->pool, id);
    return 0;
}

int
package_py_cmp(_PackageObject *self, _PackageObject *other)
{
    long cmp = package_cmp(self->package, other->package);
    if (cmp > 0)
	cmp = 1;
    else if (cmp < 0)
	cmp = -1;
    return cmp;
}

static PyObject *
package_str(_PackageObject *self)
{
    return PyString_FromFormat("<_hawkey.Package object, id: %d>", self->package->id);
}

/* getsetters */

static PyObject *
get_int(_PackageObject *self, void *closure)
{
    int (*func)(Package);
    func = (int (*)(Package))closure;
    return PyInt_FromLong(func(self->package));
}

static PyObject *
get_str(_PackageObject *self, void *closure)
{
    const char *(*func)(Package);
    const char *cstr;

    func = (const char *(*)(Package))closure;
    cstr = func(self->package);
    return PyString_FromString(cstr);
}

static PyObject *
get_str_alloced(_PackageObject *self, void *closure)
{
    char *(*func)(Package);
    char *cstr;
    PyObject *ret;

    func = (char *(*)(Package))closure;
    cstr = func(self->package);
    if (cstr == NULL)
	return PyString_FromString("");
    ret = PyString_FromString(cstr);
    solv_free(cstr);
    return ret;
}

static PyGetSetDef package_getsetters[] = {
    {"name", (getter)get_str, NULL, NULL, (void *)package_get_name},
    {"arch", (getter)get_str, NULL, NULL, (void *)package_get_arch},
    {"evr",  (getter)get_str, NULL, NULL, (void *)package_get_evr},
    {"location",  (getter)get_str_alloced, NULL, NULL,
     (void *)package_get_location},
    {"reponame",  (getter)get_str, NULL, NULL, (void *)package_get_reponame},
    {"size", (getter)get_int, NULL, NULL, (void *)package_get_size},
    {"medianr", (getter)get_int, NULL, NULL, (void *)package_get_medianr},
    {NULL}			/* sentinel */
};

/* object methods */

static PyObject *
evr_cmp(_PackageObject *self, PyObject *other)
{
    Package pkg2 = packageFromPyObject(other);
    if (pkg2 == NULL)
	return NULL;
    return PyInt_FromLong(package_evr_cmp(self->package, pkg2));
}

static PyObject *
obsoletes_list(_PackageObject *self, PyObject *unused)
{
    PackageList plist;
    PyObject *list;
    Sack csack =  sackFromPyObject(self->sack);

    if (!csack)
	return NULL;
    plist = packagelist_of_obsoletes(csack, self->package);
    list = packagelist_to_pylist(plist, self->sack);
    packagelist_free(plist);
    return list;
}

static struct PyMethodDef package_methods[] = {
    {"evr_cmp", (PyCFunction)evr_cmp, METH_O, NULL},
    {"obsoletes_list", (PyCFunction)obsoletes_list, METH_NOARGS, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject package_Type = {
    PyObject_HEAD_INIT(NULL)
    0,				/*ob_size*/
    "_hawkey.Package",		/*tp_name*/
    sizeof(_PackageObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) package_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    (cmpfunc)package_py_cmp,	/*tp_compare*/
    (reprfunc)package_str,	/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    0,				/*tp_hash */
    0,				/*tp_call*/
    (reprfunc)package_str,	/*tp_str*/
    0,				/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,		/*tp_flags*/
    "Package object",		/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    PyObject_SelfIter,		/* tp_iter */
    0,                         	/* tp_iternext */
    package_methods,		/* tp_methods */
    0,				/* tp_members */
    package_getsetters,		/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    (initproc)package_init,	/* tp_init */
    0,				/* tp_alloc */
    package_new,		/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};