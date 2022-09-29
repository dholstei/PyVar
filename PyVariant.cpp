// 
// PyVariant DSO/DLL library (Master Controller)
// 
// Author:	Danny Holstein
// 
// Desc:    Convert Variant++ objects to Python (numeric or string) objects and back again
//
#define		PYVAR_VERSION "PyVariant++-0.0"

//#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "extcode.h"

#include "PyVariant.h"

using namespace std;

#if 1   //  LabVIEW stuff
#define LStrString(A) string((char*) (*A)->str, (*A)->cnt)
LStrHandle LVStr(string str)    //  convert string to new LV string handle
{
    if (str.length() == 0) return NULL;
    LStrHandle l; if ((l = (LStrHandle)DSNewHClr(sizeof(int32) + str.length())) == NULL) return NULL;
    memmove((char*)(*l)->str, str.c_str(), ((*l)->cnt = str.length()));
    return l;
}
LStrHandle  LVStr(string str, int size)
{
    if (size == 0) return NULL;
    LStrHandle l; if ((l = (LStrHandle)DSNewHClr(sizeof(int32) + size)) == NULL) return NULL;
    memmove((char*)(*l)->str, str.c_str(), ((*l)->cnt = size));
    return l;
}
LStrHandle  LVStr(char* str, int size)
{
    if (size == 0) return NULL;
    LStrHandle l; if ((l = (LStrHandle)DSNewHClr(sizeof(int32) + size)) == NULL) return NULL;
    memmove((char*)(*l)->str, str, ((*l)->cnt = size));
    return l;
}
void        LVStr(LStrHandle loc, string* str)    //  convert string to new LV string handle
{
    if (str == NULL)
        {DSSetHandleSize(loc, sizeof(int32)); (*loc)->cnt = 0;}
    else
       {DSSetHandleSize(loc, sizeof(int32) + str == NULL ? 0 : str->length());
        memmove((char*)(*loc)->str, str->c_str(), ((*loc)->cnt = str->length()));}
}

#endif //   LabVIEW stuff

#if 1 // Python stuff
static PyObject*sys = NULL, *io = NULL, *stringio = NULL, 
                *stringioinstance = NULL, * MainModule = NULL;
static bool IsPythonInit = false;
int setup_stderr() {
    PyObject* io = NULL, * stringio = NULL, * stringioinstance = NULL;

    int success = 0;

    io = PyImport_ImportModule("io");
    if (!io) goto done;
    stringio = PyObject_GetAttrString(io, "StringIO");
    if (!stringio) goto done;
    stringioinstance = PyObject_CallFunctionObjArgs(stringio, NULL);
    if (!stringioinstance) goto done;

    if (PySys_SetObject("stderr", stringioinstance) == -1) goto done;

    success = 1;

done:
    Py_XDECREF(stringioinstance);
    Py_XDECREF(stringio);
    Py_XDECREF(io);
    return success;
}

LStrHandle get_stderr_text() {
    PyObject* pystderr = PySys_GetObject("stderr"); // borrowed reference

    PyObject* value = NULL, * encoded = NULL;

    LStrHandle result = NULL;
    char *temp_result = NULL;
    Py_ssize_t size = 0;

    value = PyObject_CallMethod(pystderr, "getvalue", NULL);
    if (!value) goto done;
    // ideally should get the preferred encoding
    encoded = PyUnicode_AsEncodedString(value, "utf-8", "strict");
    if (!encoded) goto done;
    if (PyBytes_AsStringAndSize(encoded, &temp_result, &size) == -1) goto done;
    size += 1;

    // copy so we own the memory
    result = LVStr((char*) temp_result, size);

done:
    Py_XDECREF(encoded);
    Py_XDECREF(value);
    PyErr_Clear();
    MessageBoxA(0, temp_result, "error", 0);
    return result;
}
typedef struct {
    int32_t dimSize;
    uint32_t ptrVarObj[1];
} PtrArray;
typedef PtrArray** PtrArrayHdl;
#endif  // end Python stuff

#if 1 // PyVar methods
PyVar::PyVar(VarObj* d) {   //  Python variable object from C++ Variant++
    addr = this;
    if (!IsVariant(d))
        {errnum = -1; errstr = new string("Invalid Variant++ object, use \"IsVariant()\" for details");}
    else {
        switch (d->data.index())
        {   //  good Variant++ object, create PyObject (numeric or string)
        case VarIdx::I32:   //  Python isn't strongly typed, I believe 8/16 bit is a relic so won't support it
            var = PyLong_FromLong((long)get<int32_t>(d->data));
            break;
        case VarIdx::DBL:   //  Easier to support double only, will ignore single precision
            var = PyFloat_FromDouble((double)get<double>(d->data));
            break;
        case VarIdx::Str:
            var = PyUnicode_FromString((char*)get<string*>(d->data)->c_str());
            break;
        default:
            {errnum = -1; errstr = new string("Unsupported data type"); errdata = new string(*d->name);}
            break;
        }
        if (var != NULL)
        {   //  PyObject is valid, increment reference count, then set reference name
            Py_INCREF(var);
            if (d->name != NULL)
                name = new string(d->name->c_str());    //  set name/reference in object
        }
    }
}

PyVar::~PyVar() {
    delete errstr;  errstr  = NULL;
    delete errdata; errdata = NULL;
    delete name; name = NULL;
    Py_DECREF(var);
}

bool PyVar::operator<  (const PyVar rhs) const { return addr <  rhs.addr; }
bool PyVar::operator<= (const PyVar rhs) const { return addr <= rhs.addr; }
bool PyVar::operator== (const PyVar rhs) const { return addr == rhs.addr; }
#endif // PyVar methods

#if 0 // PyModule methods
PyModule::PyModule(char* d) {   //  Python object from C++ Variant++
    addr = this;
    module = PyImport_AddModule(d);
    if (module != NULL)
    {   //  PyObject is valid, increment reference count, then set reference name
        Py_INCREF(module);
        int rc;
        if (sys == NULL) {
            errnum = -1; errstr = new string("No Python \"sys\", couldn't set name");
            errdata = new string(d);
        }
        else {
            if ((rc = PyObject_SetAttrString(sys, d, module)) != 0) {
                errnum = rc; errstr = new string("Couldn't set module name using \"PyObject_SetAttrString()\"");
                errdata = new string(d);
            }
            else name = new string(d);    //  set name/reference in object
        }
    }
}

PyModule::~PyModule() {
    delete errstr;  errstr = NULL;
    delete errdata; errdata = NULL;
    delete name; name = NULL;
    Py_DECREF(module);
}

bool PyModule::operator<  (const PyModule rhs) const { return addr < rhs.addr; }
bool PyModule::operator<= (const PyModule rhs) const { return addr <= rhs.addr; }
bool PyModule::operator== (const PyModule rhs) const { return addr == rhs.addr; }
#endif // PyModule methods

static std::list<PyVar*> PyVarObjs;     //  Python variables, numeric and string
static std::list<PyObject*> PyObjObjs;     //  Python variables, numeric and string
MSEXPORT void AddToPyList(PyVar* V) { PyVarObjs.push_back(V); }
MSEXPORT void AddToPyObjects(PyObject* V) { PyObjObjs.push_back(V); }

static struct {bool err = false; string* str = NULL;} ObjErr; //  where we store user-checked/non-API error info

bool IsPyVar(PyVar* addr) //  check for corruption/validity, use <list> to track all open control/monitor objects, avoid SEGFAULT
{
    if (addr == NULL) 
        {ObjErr.str = new string("NULL Python variable"); ObjErr.err = true; return false; }
    bool b = false;
    for (auto i : PyVarObjs) { if (i->addr == addr) {b = true; break;}}
    if (!b) 
        {ObjErr.str = new string("Invalid Python variable (unallocated memory or non-Python object)"); ObjErr.err = true; return false; }

    if (addr->addr == addr && addr->canary_end == MAGIC)
        { ObjErr.err = false; delete ObjErr.str; return true; }
    else { ObjErr.err = true; ObjErr.str = new string("Control/monitor memory corrupted"); return false; }
}

bool IsPyObject(PyObject* addr) //  check for validity, use <list> to track all Python objects, avoid SEGFAULT
{
    if (addr == NULL)
        {ObjErr.str = new string("NULL Python Object"); ObjErr.err = true; return false;}
    bool b = false;
    for (auto i : PyObjObjs) { if (i == addr) { b = true; break; } }
    if (b) return true;
    else
        {ObjErr.str = new string("Invalid Python object (unallocated memory or non-Python object)"); ObjErr.err = true; return false;}
}

extern "C" {  //  functions to be called from LabVIEW.  'extern "C"' is necessary to prevent overload name mangling

#define LStrString(A) string((char*) (*A)->str, (*A)->cnt)
MSEXPORT int PyInit() { //  initialize Python interpreter
    if (!Py_IsInitialized())
    {
        Py_InitializeEx(0);
        sys = PyImport_ImportModule("sys");
        //MainModule = PyImport_ImportModule("__main__");
        if (setup_stderr() == 0) return -1;
    }
    return 0;
}

#define DICT_ERR(e) {error->errnum = -1; error->errstr = LVStr(e); return NULL;}
MSEXPORT PyObject* PyRun(LStrHandle cmd, int start, PyObject* globals, PyObject* locals, tLvVarErr* error) {  //  run Python program
    PyObject* rtn = NULL; int rc = 0;
    if (globals != NULL) if (!IsPyObject(globals)) DICT_ERR("bad/invalid global dictionary");
    if (locals  != NULL) if (!IsPyObject(locals))  DICT_ERR("bad/invalid locals dictionary");
    //MainModule = PyImport_AddModule("__main__"); Py_INCREF(MainModule);
    if (Py_IsInitialized())
        if (globals == NULL && locals == NULL) rc = PyRun_SimpleString(LStrString(cmd).c_str());
        else {rtn = PyRun_String(LStrString(cmd).c_str(), start, globals, locals);}
    else { error->errnum = -1;  error->errstr = LVStr("Python not initialized"); return NULL;}

    if (rtn == NULL || rc != 0)
    {   //  this is the error condition, return value is NULL
        PyErr_Print();
        error->errnum = -1;
        LStrHandle errstr = get_stderr_text();
        if (LStrString(cmd).size() > 0) error->errdata = LVStr(LStrString(cmd));
        if (errstr != NULL) error->errstr = errstr;
        else error->errstr = LVStr("Unknown error");
    }
    return rtn;
}

MSEXPORT PyVar* PyVarCreate(VarObj* data, tLvVarErr* error) {   //  create Python variable object
    if (!Py_IsInitialized())    //  no place to set the name attribute
        {ObjErr.err = true; ObjErr.str = new string("Python interpreter not initialized"); return NULL;}   
    if (!IsVariant(data)) { //  check for valid Variant, on error, get object error
        tObjErr* VarErr = (tObjErr*) GetObjErr();   //  get error from Variant library
        error->errnum = VarErr->err; VarErr->err = 0;
        if (VarErr->str != NULL)
            {error->errstr = LVStr(*VarErr->str); delete VarErr->str; VarErr->str = NULL;}
        return NULL;}

    PyVar *pValue =  new PyVar(data);
    if (pValue->errnum) {
        error->errnum = pValue->errnum;
        if (pValue->errdata != NULL) error->errdata = LVStr(*(pValue->errdata));
        if (pValue->errstr != NULL) error->errstr = LVStr(*(pValue->errstr));
        else error->errstr = LVStr("Unknown error");
    }
    else AddToPyList(pValue);   //  SUCCESS, add to list to track.
    return pValue;
}

MSEXPORT PyObject* GetPyObject(PyVar* var) {return var->var;}

MSEXPORT void GetPyVarName(LStrHandle name, PyVar* var)  {LVStr(name, var->name);}

MSEXPORT int PyVarDelete(PyVar* var) { //  delete LV Python variable object
    if (!IsPyVar(var)) return -1;  //  don't want to risk a dump in case of invalid data
    PyVarObjs.remove(var); delete var;
    return 0;
}

MSEXPORT PyObject* PyDictCreate(PtrArrayHdl PyObjects, PyObject* m, tLvVarErr* error) {  //  create Python dictionary
    //  NOTE:   This is the means to transfer in/out between Python variables and C++ variant objects
    //          Add items to mudule if not NULL
    if (!Py_IsInitialized())
        {ObjErr.err = true; ObjErr.str = new string("Python interpreter not initialized"); return NULL;}
    if ((*PyObjects)->dimSize == 0) return NULL;
    PyObject* pValue;
    if (m == NULL) pValue = PyDict_New();
    else pValue = PyModule_GetDict(m);

    if (pValue == NULL) DICT_ERR("Couldn't create Python dictionary");
    for (size_t i = 0; i < (*PyObjects)->dimSize; i++)
    {
        PyVar* it = (PyVar *) (*PyObjects)->ptrVarObj[i];
        if (PyDict_SetItemString(pValue, it->name->c_str(), it->var))
            {error->errdata = LVStr(*it->name); DICT_ERR("Couldn't add Python dictionary item");}
    }
    AddToPyObjects(pValue);   //  SUCCESS, add to list to track.
    return pValue;
}

MSEXPORT int PyDictDelete(PyObject* dict) { //  delete Python dictionary object
    if (!IsPyObject(dict)) return -1;  //  don't want to risk a dump in case of invalid data
    PyObjObjs.remove(dict); Py_DECREF(dict);
    return 0;
}

MSEXPORT VarObj* PyVarToVarPP(char* name, PyObject* var) {  //  convert Python variable object to Variant++ (C++) object
    if (var == NULL) return NULL;  //  don't want to risk a dump in case of invalid data
    VarObj* V = NULL;
    string* type = new string(var->ob_type->tp_name);
    bool err = false; PyObject* ErrObj = NULL;    //  so we may parse the error later
    if (*type == string("int")){
        int32_t val = PyLong_AsLong(var);
        if (val == -1) err = (ErrObj = PyErr_Occurred()) != NULL;
        if (!err) {V = new VarObj(string(name), val); AddToVarList(V); return V;}
        else {ObjErr.err = true; ObjErr.str = new string("Couldn't convert integer"); return NULL; }
        }
    if (*type == string("float")){
        double val = PyFloat_AsDouble(var);
        if (val == -1) err = (ErrObj = PyErr_Occurred()) != NULL;
        if (!err) {V = new VarObj(string(name), val); AddToVarList(V); return V; }
        else {ObjErr.err = true; ObjErr.str = new string("Couldn't convert float"); return NULL; }
        }
    if (*type == string("str")) {
        PyObject *asstring; char* bytearray;
        asstring = PyByteArray_FromObject(var);
        bytearray = PyByteArray_AsString(asstring);
        string* val = &string(bytearray);
        if (val == NULL) err = (ErrObj = PyErr_Occurred()) != NULL;
        if (!err) { V = new VarObj(string(name), val); AddToVarList(V); return V; }
        else { ObjErr.err = true; ObjErr.str = new string("Couldn't convert string"); return NULL; }
    }

}

#if 1   //  utility-ish functions

MSEXPORT void GetError(PyVar* PyVarObj, tLvVarErr* error) { //  get error info
    if (PyVarObj == NULL || ObjErr.err) { //  NULL object, or error in object creation
        if (!ObjErr.err) return; //  no error
        error->errnum = -1;
        error->errstr = LVStr(*ObjErr.str);
        ObjErr.err = false; delete ObjErr.str; //  Clear error, but race conditions may exist, if so, da shit has hit da fan. 
    }
    else
    {   //  API error info is stored in object, pass up to caller
        error->errnum = PyVarObj->errnum;
        if (PyVarObj->errdata != NULL) error->errdata = LVStr(*(PyVarObj->errdata));
        if (PyVarObj->errstr != NULL) error->errstr = LVStr(*(PyVarObj->errstr));
        else error->errstr = LVStr("Unknown error");
        PyVarObj->errnum = 0;
        delete PyVarObj->errstr;  PyVarObj->errstr  = NULL;
        delete PyVarObj->errdata; PyVarObj->errdata = NULL;
    }
}

MSEXPORT void Version(char* buf, int sz) {   //  get version string
    memcpy(buf, string(PYVAR_VERSION).c_str(), min<int>(sz, string(PYVAR_VERSION).length()));}

#endif //   end utility-ish functions

}   //  end extern "C"