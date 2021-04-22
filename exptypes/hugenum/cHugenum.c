#define NPY_NO_DEPRECATED_API NPY_API_VERSION
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"

#include "hugenum.h"

#define HUGENUM_X_PYTYPE T_DOUBLE
#define HUGENUM_E_PYTYPE T_UBYTE

#define UNARYFUNC_BOOL_DECL(name) \
static PyObject * Hugenum_##name(PyObject * o1, PyObject * unused); \
static int Hugenum_##name##_op(PyObject * o1);
#define UNARYFUNC_DECL(name) \
static PyObject * Hugenum_##name(PyObject * o1, PyObject * unused); \
static PyObject * Hugenum_##name##_op(PyObject * o1);
#define BINARYFUNC_DECL(name) \
static PyObject * Hugenum_##name(PyObject * o1, PyObject * o2); \
static PyObject * Hugenum_##name##_op(PyObject * o1, PyObject * o2);

static PyObject * Hugenum_richcompare(PyObject *o1, PyObject *o2, int op);

static PyObject * Hugenum_repr(PyObject *self);
static PyObject * Hugenum_str(PyObject *self);

static int Hugenum_init(PyObject *self, PyObject *args, PyObject *kwds);

UNARYFUNC_DECL(from_scalar);
UNARYFUNC_DECL(to_scalar);

UNARYFUNC_BOOL_DECL(iszero);
UNARYFUNC_BOOL_DECL(isnonzero);
UNARYFUNC_BOOL_DECL(isnan);
UNARYFUNC_BOOL_DECL(isinf);
UNARYFUNC_BOOL_DECL(isfinite);

UNARYFUNC_DECL(sign);

UNARYFUNC_DECL(absolute);
UNARYFUNC_DECL(log);
UNARYFUNC_DECL(log10);
UNARYFUNC_DECL(exp);
UNARYFUNC_DECL(exp10);
UNARYFUNC_DECL(negative);
UNARYFUNC_DECL(normalized);

BINARYFUNC_DECL(add)
BINARYFUNC_DECL(subtract)
BINARYFUNC_DECL(multiply)
BINARYFUNC_DECL(divide)
BINARYFUNC_DECL(copysign)

static PyObject * Hugenum_power(PyObject *o1, PyObject *args[], int argc); // member function version
static PyObject * Hugenum_power_op(PyObject * b, PyObject * e, PyObject * m); // operator version

typedef struct {
    PyObject_HEAD
    hugenum v;
} Hugenum;

static PyMemberDef Hugenum_members[] = {
    {"x", HUGENUM_X_PYTYPE, offsetof(Hugenum, v.x), 0,
     "mantissa"},
    {"e", HUGENUM_E_PYTYPE, offsetof(Hugenum, v.e), 0,
     "exponent"},
    {"p", T_BOOL, offsetof(Hugenum, v.p), 0,
     "sign"},
    {NULL}  /* Sentinel */
};


#define UNARYFUNC_METH(name) {#name, (PyCFunction) Hugenum_##name, METH_NOARGS, ""}
#define BINARYFUNC_METH(name) {#name, (PyCFunction) Hugenum_##name, METH_O, ""}

static PyMethodDef Hugenum_methods[] = {
    UNARYFUNC_METH(to_scalar),
    {"from_scalar", (PyCFunction) Hugenum_from_scalar, METH_O | METH_CLASS, ""},

    UNARYFUNC_METH(iszero),
    UNARYFUNC_METH(isnonzero),
    UNARYFUNC_METH(isnan),
    UNARYFUNC_METH(isinf),
    UNARYFUNC_METH(isfinite),

    UNARYFUNC_METH(sign),
    
    UNARYFUNC_METH(absolute),
    UNARYFUNC_METH(log),
    UNARYFUNC_METH(log10),
    UNARYFUNC_METH(exp),
    UNARYFUNC_METH(exp10),
    UNARYFUNC_METH(negative),
    UNARYFUNC_METH(normalized),

    BINARYFUNC_METH(add),
    BINARYFUNC_METH(subtract),
    BINARYFUNC_METH(multiply),
    BINARYFUNC_METH(divide),
    BINARYFUNC_METH(copysign),

    {"power", (PyCFunction) Hugenum_power, METH_FASTCALL, ""},

    {NULL}  /* Sentinel */
};

static PyNumberMethods Hugenum_number_methods = {
    .nb_add = Hugenum_add_op,
    .nb_subtract = Hugenum_subtract_op,
    .nb_multiply = Hugenum_multiply_op,
    .nb_true_divide = Hugenum_divide_op,
    .nb_float = Hugenum_to_scalar_op,
    .nb_absolute = Hugenum_absolute_op,
    .nb_negative = Hugenum_negative_op,
    .nb_positive = Hugenum_normalized_op,
    .nb_power = Hugenum_power_op,
};

static PyTypeObject HugenumType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "hugenum.Hugenum",
    .tp_doc = "Hugenum object.",
    .tp_basicsize = sizeof(Hugenum),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_members = Hugenum_members,
    .tp_methods = Hugenum_methods,
    .tp_richcompare = Hugenum_richcompare,
    .tp_as_number = &Hugenum_number_methods,
    .tp_repr = Hugenum_repr,
    .tp_str = Hugenum_str,
    .tp_init = Hugenum_init,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
};

static PyMethodDef HugenumMethods[] = {
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef cHugenum =
{
    PyModuleDef_HEAD_INIT,
    "cHugenum", /* name of module */
    "",          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    HugenumMethods
};

PyMODINIT_FUNC
PyInit_cHugenum(void)
{
    PyObject *m;

    if (PyType_Ready(&HugenumType) < 0)
        return NULL;
    
    m = PyModule_Create(&cHugenum);
    if (m == NULL) return NULL;

    Py_INCREF(&HugenumType);
    if (PyModule_AddObject(m, "Hugenum", (PyObject *) &HugenumType) < 0) {
        Py_DECREF(&HugenumType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}



#define UNARYFUNC_BOOL_IMPL(name) \
static PyObject * \
Hugenum_##name(PyObject * o1, PyObject * Py_UNUSED(ignored)) { \
    bool r = hugenum_##name(((Hugenum *)o1)->v); \
    if (r) { \
        Py_RETURN_TRUE; \
    } else { \
        Py_RETURN_FALSE; \
    } \
} \
int \
Hugenum_##name##_op(PyObject * o1) { \
    return hugenum_##name(((Hugenum *)o1)->v); \
}

#define UNARYFUNC_HUGE_IMPL(name) \
static PyObject * \
Hugenum_##name(PyObject * o1, PyObject * Py_UNUSED(ignored)) { \
    Hugenum *r = PyObject_New(Hugenum, &HugenumType); \
    r->v = hugenum_##name(((Hugenum *)o1)->v); \
    return (PyObject *) r; \
}\
static PyObject * \
Hugenum_##name##_op(PyObject * o1) { \
    Hugenum *r = PyObject_New(Hugenum, &HugenumType); \
    r->v = hugenum_##name(((Hugenum *)o1)->v); \
    return (PyObject *) r; \
}

#define BINARYFUNC_HUGE_IMPL(name) \
static PyObject * \
Hugenum_##name(PyObject * o1, PyObject * o2) { \
    Hugenum *r = PyObject_New(Hugenum, &HugenumType); \
    r->v = hugenum_##name(((Hugenum *)o1)->v, ((Hugenum *)o2)->v); \
    return (PyObject *) r; \
} \
static PyObject * \
Hugenum_##name##_op(PyObject * o1, PyObject * o2) { \
    Hugenum *r = PyObject_New(Hugenum, &HugenumType); \
    r->v = hugenum_##name(((Hugenum *)o1)->v, ((Hugenum *)o2)->v); \
    return (PyObject *) r; \
}

static PyObject *
Hugenum_richcompare(PyObject *o1, PyObject *o2, int op) {
    bool r;
    switch(op){
        case Py_LT: r = hugenum_less(((Hugenum *)o1)->v, ((Hugenum *)o2)->v); break;
        case Py_LE: r = hugenum_less_equal(((Hugenum *)o1)->v, ((Hugenum *)o2)->v); break;
        case Py_EQ: r = hugenum_equal(((Hugenum *)o1)->v, ((Hugenum *)o2)->v); break;
        case Py_NE: r = hugenum_not_equal(((Hugenum *)o1)->v, ((Hugenum *)o2)->v); break;
        case Py_GT: r = hugenum_less(((Hugenum *)o2)->v, ((Hugenum *)o1)->v); break;
        case Py_GE: r = hugenum_less_equal(((Hugenum *)o2)->v, ((Hugenum *)o1)->v); break;
    }
    if (r) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject *
Hugenum_repr(PyObject *self) {
    char buf[128];
    hugenum q = ((Hugenum *)self)->v;
    snprintf(buf, 128, "Hugenum(%g, %d, %s)", q.x, q.e, q.p?"True":"False");
    return PyUnicode_FromString(buf);
}

#define HUGENUM_STR_LEN 9
static PyObject *
Hugenum_str(PyObject *self) {
    char buf[HUGENUM_STR_LEN];
    const size_t len = HUGENUM_STR_LEN;
    size_t idx = 0;
    
    hugenum q = ((Hugenum *)self)->v;

    if (!q.p) {
        idx += snprintf(&buf[idx], len-idx, "-");
    }
    for (int i = 0; i<q.e; i++) {
        idx += snprintf(&buf[idx], len-idx, "e");
    }
    idx += snprintf(&buf[idx], len-idx, "%g", q.x);
    return PyUnicode_FromString(buf);
}

static int 
Hugenum_init(PyObject *self, PyObject *args, PyObject *kwds) {
    hugenum q = {0.0, 0, true};
    if (!PyArg_ParseTuple(args, "|dbp", &q.x, &q.e, &q.p))
        return -1;
    ((Hugenum *)self)->v = q;
    return 0;
}

static PyObject *
Hugenum_from_scalar(PyObject * cls, PyObject * o1) {
    double x = PyFloat_AsDouble(o1);
    Hugenum *r = PyObject_New(Hugenum, &HugenumType);
    r->v = hugenum_from_scalar(x);
    return (PyObject *) r;
}

static PyObject *
Hugenum_to_scalar(PyObject * o1, PyObject * Py_UNUSED(ignored)) {
    double r = hugenum_to_scalar(((Hugenum *)o1)->v);
    return PyFloat_FromDouble(r);
}

static PyObject *
Hugenum_to_scalar_op(PyObject * o1) {
    double r = hugenum_to_scalar(((Hugenum *)o1)->v);
    return PyFloat_FromDouble(r);
}

UNARYFUNC_BOOL_IMPL(iszero)
UNARYFUNC_BOOL_IMPL(isnonzero)
UNARYFUNC_BOOL_IMPL(isnan)
UNARYFUNC_BOOL_IMPL(isinf)
UNARYFUNC_BOOL_IMPL(isfinite)

static PyObject *
Hugenum_sign(PyObject * o1, PyObject * Py_UNUSED(ignored)) {
    int s = hugenum_sign(((Hugenum *)o1)->v);
    return PyLong_FromLong(s);
}

UNARYFUNC_HUGE_IMPL(absolute)
UNARYFUNC_HUGE_IMPL(log)
UNARYFUNC_HUGE_IMPL(log10)
UNARYFUNC_HUGE_IMPL(exp)
UNARYFUNC_HUGE_IMPL(exp10)
UNARYFUNC_HUGE_IMPL(negative)
UNARYFUNC_HUGE_IMPL(normalized)

BINARYFUNC_HUGE_IMPL(add)
BINARYFUNC_HUGE_IMPL(subtract)
BINARYFUNC_HUGE_IMPL(multiply)
BINARYFUNC_HUGE_IMPL(divide)
BINARYFUNC_HUGE_IMPL(copysign)

static PyObject *
Hugenum_power(PyObject *o1, PyObject *args[], int argc) {
    Hugenum *r = PyObject_New(Hugenum, &HugenumType);
    r->v = hugenum_power(((Hugenum *)o1)->v, ((Hugenum *)args[0])->v);
    return (PyObject *) r;
}
static PyObject *
Hugenum_power_op(PyObject * b, PyObject * e, PyObject * m) {
    Hugenum *r = PyObject_New(Hugenum, &HugenumType);
    r->v = hugenum_power(((Hugenum *)b)->v, ((Hugenum *)e)->v);
    return (PyObject *) r;
}