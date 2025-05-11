#include <Python.h>
#include "../../include/plugin.h"
#include "../signals/signals.h"

// ==========================
// Globals
// ==========================
static CoreContext *python_ctx = NULL;              // Saved CoreContext
static PyObject *python_signal_callback_map = NULL; // { signal_name: [callback_list] }
static PyObject *py_module = NULL;
static PyObject *py_update = NULL;
static int update_ref_exists = 0;

static void (*signal_emit_fn)(CoreContext *, const char *, void *, void *) = NULL;
static void (*signal_connect_fn)(const char *, void (*)(CoreContext *, void *, void *, void *), void *) = NULL;

// Forward C signals into Python callbacks
static void python_signal_dispatcher(CoreContext *ctx, void *sender, void *args, void *user_data)
{
    (void)ctx;
    const char *signal_name = (const char *)user_data;

    if (!python_signal_callback_map)
        return;

    PyObject *list = PyDict_GetItemString(python_signal_callback_map, signal_name);
    if (!list)
        return;

    for (Py_ssize_t i = 0; i < PyList_Size(list); ++i)
    {
        PyObject *cb = PyList_GetItem(list, i); // Borrowed ref
        if (cb && PyCallable_Check(cb))
        {
            PyObject *py_sender = sender ? (PyObject *)sender : Py_None;
            PyObject *py_args = args ? (PyObject *)args : Py_None;
            Py_INCREF(py_sender);
            Py_INCREF(py_args);

            PyObject *result = PyObject_CallFunctionObjArgs(cb, py_sender, py_args, NULL);
            if (!result)
            {
                PyErr_Print();
            }
            Py_XDECREF(result);
            Py_DECREF(py_sender);
            Py_DECREF(py_args);
        }
    }
}

// ==========================
// Core exposed functions
// ==========================

/**
 * @brief Log a message to the engine console.
 *
 * Python Usage:
 *     core.log(message: str)
 *
 * Logs a string message using the engine's core logger.
 * 
 * @param self  Unused.
 * @param args  Tuple containing one string argument.
 *
 * @return None.
 */
static PyObject* py_log(PyObject* self, PyObject* args)
{
    (void)self;
    const char *msg;
    if (!PyArg_ParseTuple(args, "s", &msg))
        return NULL;

    if (python_ctx)
        python_ctx->log(LL_INFO, "[python] %s", msg);

    Py_RETURN_NONE;
}

/**
 * @brief Emit a signal from Python.
 *
 * Python Usage:
 *     core.signal_emit(name: str, sender: object, args: object)
 *
 * Emits a signal to the engine's signal system.
 * 
 * Important:
 * - If sender and args are None, the signal is safe for both C and Python listeners.
 * - If sender and args are Python objects (dicts, custom classes), only Python listeners should be connected.
 * - C listeners expect NULL for sender and args unless otherwise explicitly handled.
 *
 * @param self  Unused.
 * @param args  Tuple containing signal name, sender, and args.
 *
 * @return None.
 */
static PyObject* py_signal_emit(PyObject* self, PyObject* args)
{
    (void)self;
    const char *signal_name;
    PyObject *sender = NULL;
    PyObject *signal_args = NULL;

    if (!PyArg_ParseTuple(args, "s|OO", &signal_name, &sender, &signal_args))
        return NULL;

    if (signal_emit_fn && python_ctx)
    {
        signal_emit_fn(python_ctx, signal_name, sender, signal_args);
    }
    else if (python_ctx)
    {
        python_ctx->log(LL_ERROR, "[python] signal_emit called but Signals plugin is not loaded.");
    }

    Py_RETURN_NONE;
}

/**
 * @brief Connect a Python function to a signal.
 *
 * Python Usage:
 *     core.signal_connect(name: str, callback: callable)
 *
 * Connects a Python callable to a signal.
 * 
 * Important:
 * - The callback must accept two arguments: sender and args.
 * - If a C plugin emits the signal and passes non-NULL pointers as args,
 *   these are raw addresses in Python and unsafe to use unless understood.
 * - Safe signals pass sender=None, args=None.
 *
 * @param self  Unused.
 * @param args  Tuple containing signal name and callback.
 *
 * @return None.
 */
static PyObject* py_signal_connect(PyObject* self, PyObject* args)
{
    (void)self;
    const char *signal_name;
    PyObject *callback;

    if (!PyArg_ParseTuple(args, "sO", &signal_name, &callback))
        return NULL;

    if (!PyCallable_Check(callback))
    {
        PyErr_SetString(PyExc_TypeError, "Callback must be callable");
        return NULL;
    }

    if (!python_signal_callback_map)
    {
        python_signal_callback_map = PyDict_New();
    }

    PyObject *list = PyDict_GetItemString(python_signal_callback_map, signal_name);
    if (!list)
    {
        list = PyList_New(0);
        PyDict_SetItemString(python_signal_callback_map, signal_name, list);
    }

    PyList_Append(list, callback);

    if (signal_connect_fn)
    {
        signal_connect_fn(signal_name, python_signal_dispatcher, (void *)signal_name);
    }
    else if (python_ctx)
    {
        python_ctx->log(LL_ERROR, "[python] signal_connect called but Signals plugin is not loaded.");
    }

    Py_RETURN_NONE;
}


/**
 * @brief Retrieve a value from the engine's memory map.
 *
 * Python Usage:
 *     core.get(key: str) -> int
 *
 * Fetches a value from the core's memory map by key.
 * 
 * Important:
 * - Only safe to retrieve memory entries that are integers (int).
 * - Retrieving complex structs, floats, or raw pointers is unsupported and unsafe.
 * - Future versions may expand type handling.
 *
 * @param self  Unused.
 * @param args  Tuple containing memory key string.
 *
 * @return Python int if the memory entry is a simple int, else a PyCapsule or None.
 */
static PyObject* py_memory_get(PyObject* self, PyObject* args)
{
    (void)self;
    const char *key;
    if (!PyArg_ParseTuple(args, "s", &key))
        return NULL;

    if (!python_ctx)
        Py_RETURN_NONE;

    void *ptr = CC_GET(python_ctx, (char *)key);
    if (!ptr)
        Py_RETURN_NONE;

    size_t size = mm_get_size(&python_ctx->memory.map, STR((char *)key));

    if (size == sizeof(int))
    {
        int value = *(int *)ptr;
        return PyLong_FromLong(value);
    }

    // Otherwise fallback, maybe return a capsule or None
    return PyCapsule_New(ptr, "memory", NULL);
}

// ==========================
// Engine Module Definitions
// ==========================

static PyMethodDef engine_methods[] = {
    {"log", (PyCFunction)py_log, METH_VARARGS,
     "log(message)\n"
     "Logs a message to the engine console.\n"
     "Equivalent to C core logging.\n"},

    {"signal_emit", (PyCFunction)py_signal_emit, METH_VARARGS,
     "signal_emit(name: str, sender: object, args: object)\n"
     "Emit a signal.\n"
     "If sender and args are None, the signal can be received safely by C or Python listeners.\n"
     "If sender and args are Python objects (e.g., dicts, custom classes),\n"
     "only Python listeners should be connected. C listeners will not understand Python objects.\n"
     "Recommended: For cross-language signals, always emit sender=None and args=None."},

    {"signal_connect", (PyCFunction)py_signal_connect, METH_VARARGS,
     "signal_connect(name: str, callback: callable)\n"
     "Connect a Python function to a signal.\n"
     "The callback must take two arguments: sender and args.\n"
     "Signals can be emitted by C or Python. If emitted by C with non-None arguments,\n"
     "the sender and args will be raw pointers, which are unsafe to use directly in Python.\n"
     "Assume sender and args are None unless you control the C side carefully."},

    {"get", (PyCFunction)py_memory_get, METH_VARARGS,
     "get(key: str) -> int\n"
     "Get a value from the engine's memory map.\n"
     "Currently only safe to retrieve memory entries that are simple integers (int).\n"
     "Do not use to retrieve complex structs, floats, or raw pointers unless you know the memory layout.\n"
     "Future versions may expand support for more types."},

    {NULL, NULL, 0, NULL}};

static struct PyModuleDef engine_module = {
    PyModuleDef_HEAD_INIT,
    "core", // Python module name
    "Core Engine Bindings",
    -1,
    engine_methods,
    NULL, NULL, NULL, NULL};

// ==========================
// Plugin API Functions
// ==========================

int init(CoreContext *ctx)
{
    python_ctx = ctx;

    Py_Initialize();

    // Expose 'core' module
    PyObject *core_mod = PyModule_Create(&engine_module);
    if (core_mod)
    {
        PyObject *sys_modules = PyImport_GetModuleDict();
        PyDict_SetItemString(sys_modules, "core", core_mod);
        Py_DECREF(core_mod);
    }

    // Try to grab signal functions
    signal_emit_fn = CC_GET(ctx, "signal::emit");
    signal_connect_fn = CC_GET(ctx, "signal::connect");

    if (!signal_emit_fn || !signal_connect_fn)
    {
        ctx->log(LL_ERROR, "[python] Warning: Signals plugin not found. signal_connect and signal_emit will not work.");
    }

    // Load scripts/init.py
    FILE *file = fopen("scripts/init.py", "r");
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

    // Import __main__ and find functions
    PyObject *py_name = PyUnicode_FromString("__main__");
    py_module = PyImport_Import(py_name);
    Py_DECREF(py_name);

    if (!py_module)
    {
        ctx->log(LL_ERROR, "[python] Failed to import __main__");
        PyErr_Print();
        return 1;
    }

    // Call optional init()
    PyObject *py_init = PyObject_GetAttrString(py_module, "init");
    if (py_init && PyCallable_Check(py_init))
    {
        PyObject *result = PyObject_CallObject(py_init, NULL);
        if (!result)
            PyErr_Print();
        Py_XDECREF(result);
    }
    Py_XDECREF(py_init);

    // Setup update()
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

int update(CoreContext *ctx)
{
    if (update_ref_exists && py_update)
    {
        PyObject *args = PyTuple_New(1);
        PyObject *dt = PyFloat_FromDouble(ctx->delta_time);
        PyTuple_SetItem(args, 0, dt);

        PyObject *result = PyObject_CallObject(py_update, args);
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

void run_python_func(CoreContext *ctx, const char *function)
{
    PyObject *func = PyObject_GetAttrString(py_module, function);
    if (func && PyCallable_Check(func))
    {
        PyObject *result = PyObject_CallObject(func, NULL);
        if (!result)
        {
            ctx->log(LL_ERROR, "[python::%s] Exception occurred", function);
            PyErr_Print();
        }
        Py_XDECREF(result);
    }
    Py_XDECREF(func);
}

int shutdown(CoreContext *ctx)
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

    if (python_signal_callback_map)
    {
        Py_DECREF(python_signal_callback_map);
        python_signal_callback_map = NULL;
    }

    if (Py_IsInitialized())
        Py_Finalize();

    return 0;
}

// Plugin API
PluginAPI Load()
{
    static const char *deps[] = {NULL};
    static const char *optional[] = {"Signals", NULL};
    static PluginMetadata meta = {"Python", deps, optional};

    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta};
}
