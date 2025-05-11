#include <Python.h>
#include "../../include/plugin.h"

static PyObject* py_module = NULL;
static PyObject* py_update = NULL;
static int update_ref_exists = 0;

static CoreContext* python_ctx = NULL; // Global for this plugin

static PyObject* py_log(PyObject* self, PyObject* args)
{
    (void)self;
    const char* msg;
    if (!PyArg_ParseTuple(args, "s", &msg))
        return NULL;

    if (python_ctx)
        python_ctx->log(LL_INFO, "[python] %s", msg);

    Py_RETURN_NONE;
}


static PyObject* make_engine_module(CoreContext* ctx)
{
    (void)ctx;
    static PyMethodDef methods[] = {
        { "log", (PyCFunction)py_log, METH_VARARGS, "Log a message" },
        { NULL, NULL, 0, NULL }
    };

    static struct PyModuleDef module_def = {
        PyModuleDef_HEAD_INIT,
        "core", // exposed as "import core"
        "Core Engine Bindings",
        -1,
        methods,
        NULL,
        NULL,
        NULL,
        NULL
    };

    return PyModule_Create(&module_def);
}

int init(CoreContext* ctx)
{
    Py_Initialize();

    python_ctx = ctx;

    // Expose core.log
    PyObject* core_module = make_engine_module(ctx);
    if (core_module)
    {
        PyObject* sys_modules = PyImport_GetModuleDict();
        PyDict_SetItemString(sys_modules, "core", core_module);
        Py_DECREF(core_module);
    }

    // Load scripts/init.py
    FILE* file = fopen("scripts/init.py", "r");
    if (!file)
    {
        ctx->log(LL_ERROR, "No scripts/init.py found for Python.");
        return 1;
    }

    if (PyRun_SimpleFile(file, "scripts/init.py") != 0)
    {
        ctx->log(LL_ERROR, "[python::init] Error running scripts/init.py");
        fclose(file);
        return 1;
    }
    fclose(file);

    // Import __main__ and get references
    PyObject* py_name = PyUnicode_FromString("__main__");
    py_module = PyImport_Import(py_name);
    Py_DECREF(py_name);

    if (!py_module)
    {
        ctx->log(LL_ERROR, "[python] Failed to import __main__");
        PyErr_Print();
        return 1;
    }

    // Optional: call init()
    PyObject* py_init = PyObject_GetAttrString(py_module, "init");
    if (py_init && PyCallable_Check(py_init))
    {
        PyObject* result = PyObject_CallObject(py_init, NULL);
        if (!result) PyErr_Print();
        Py_XDECREF(result);
    }
    Py_XDECREF(py_init);

    // Lookup update() for reuse
    py_update = PyObject_GetAttrString(py_module, "update");
    if (py_update && PyCallable_Check(py_update))
    {
        update_ref_exists = 1;
    }
    else
    {
        update_ref_exists = 0;
        Py_XDECREF(py_update);
        py_update = NULL;
    }

    return 0;
}

void run_python_func(CoreContext* ctx, const char* function)
{
    PyObject* func = PyObject_GetAttrString(py_module, function);
    if (func && PyCallable_Check(func))
    {
        PyObject* result = PyObject_CallObject(func, NULL);
        if (!result)
        {
            ctx->log(LL_ERROR, "[python::%s] Exception occurred", function);
            PyErr_Print();
        }
        Py_XDECREF(result);
    }
    Py_XDECREF(func);
}

int shutdown(CoreContext* ctx)
{
    run_python_func(ctx, "shutdown");

    if (py_update)
    {
        Py_XDECREF(py_update);
        py_update = NULL;
    }

    if (py_module)
    {
        Py_XDECREF(py_module);
        py_module = NULL;
    }

    if (Py_IsInitialized())
        Py_Finalize();

    return 0;
}

int update(CoreContext* ctx)
{
    if (update_ref_exists && py_update)
    {
        PyObject* args = PyTuple_New(1);
        PyObject* dt = PyFloat_FromDouble(ctx->delta_time);
        PyTuple_SetItem(args, 0, dt); // steals reference
        PyObject* result = PyObject_CallObject(py_update, args);
        if (!result)
        {
            ctx->log(LL_ERROR, "[python::update] Exception occurred");
            PyErr_Print();
        }
        Py_DECREF(args);
        Py_XDECREF(result);
    }

    return 0;
}

PluginAPI Load()
{
    static const char* deps[] = { NULL };
    static const char* optional[] = { NULL };
    static PluginMetadata meta = { "Python", deps, optional };

    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta
    };
}
