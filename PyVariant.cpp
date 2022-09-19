// 
// PyVariant DSO/DLL library (Master Controller)
// 
// Author:	Danny Holstein
// 
// Desc:    Convert Variant++ objects to Python (numeric or string) objects and back again
//
#define		PYVAR_VERSION "PyVariant++-0.0"

#define PY_SSIZE_T_CLEAN
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
LStrHandle LVStr(string str, int size)
{
    if (size == 0) return NULL;
    LStrHandle l; if ((l = (LStrHandle)DSNewHClr(sizeof(int32) + size)) == NULL) return NULL;
    memmove((char*)(*l)->str, str.c_str(), ((*l)->cnt = size));
    return l;
}
LStrHandle LVStr(char* str, int size)
{
    if (size == 0) return NULL;
    LStrHandle l; if ((l = (LStrHandle)DSNewHClr(sizeof(int32) + size)) == NULL) return NULL;
    memmove((char*)(*l)->str, str, ((*l)->cnt = size));
    return l;
}
#endif //   LabVIEW stuff

#if 1 // Python stuff
static PyObject*sys = NULL, *io = NULL, *stringio = NULL, 
                *stringioinstance = NULL, * module = NULL;
int setup_stderr() {
    //PyObject* io = NULL, * stringio = NULL, * stringioinstance = NULL;

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

    return result;
}
#endif  // end Python stuff

#if 1 // PyVar methods
PyVar::PyVar(VarObj* d) {   //  Python object from C++ Variant++
    addr = this;
    if (!IsVariant(d))
        {errno = -1; errstr = new string("Invalid Variant++ object, use \"IsVariant()\" for details");}
    else {
        switch (d->data.index())
        {   //  good Variant++ object, create PyObject (numeric or string)
        case VarIdx::I32:
            PyObj = PyLong_FromLong((long)get<int32_t>(d->data));
            break;
        case VarIdx::DBL:
            PyObj = PyFloat_FromDouble((double)get<double>(d->data));
            break;
        case VarIdx::Str:
            PyObj = PyUnicode_FromString((char*)get<string*>(d->data)->c_str());
            break;
        default:
            {errno = -1; errstr = new string("Unsupported data type"); errdata = new string(*d->name);}
            break;
        }
        if (PyObj != NULL)
        {   //  PyObject is valid, increment reference count, then set reference name
            Py_INCREF(PyObj);
            int rc;
            if (sys == NULL) {errno = -1; errstr = new string("No Python \"sys\", couldn't set name");
                              errdata = new string(*d->name); }
            else {
                if ((rc = PyObject_SetAttrString(sys, d->name->c_str(), PyObj)) != 0) {
                    errno = rc; errstr = new string("Couldn't set name using \"PyObject_SetAttrString()\"");
                    errdata = new string(*d->name);
                }
                else name = new string(*d->name);    //  set name/reference in object
            }
        }
    }
}

PyVar::~PyVar() {
    delete errstr;  errstr  = NULL;
    delete errdata; errdata = NULL;
    delete name; name = NULL;
    Py_DECREF(PyObj);
}

bool PyVar::operator<  (const PyVar rhs) const { return addr <  rhs.addr; }
bool PyVar::operator<= (const PyVar rhs) const { return addr <= rhs.addr; }
bool PyVar::operator== (const PyVar rhs) const { return addr == rhs.addr; }
#endif // PyVar methods

#if 1 // PyModule methods
PyModule::PyModule(char* d) {   //  Python object from C++ Variant++
    addr = this;
    PyObj = PyImport_AddModule(d);
    if (PyObj != NULL)
    {   //  PyObject is valid, increment reference count, then set reference name
        Py_INCREF(PyObj);
        int rc;
        if (sys == NULL) {
            errno = -1; errstr = new string("No Python \"sys\", couldn't set name");
            errdata = new string(d);
        }
        else {
            if ((rc = PyObject_SetAttrString(sys, d, PyObj)) != 0) {
                errno = rc; errstr = new string("Couldn't set module name using \"PyObject_SetAttrString()\"");
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
    Py_DECREF(PyObj);
}

bool PyModule::operator<  (const PyModule rhs) const { return addr < rhs.addr; }
bool PyModule::operator<= (const PyModule rhs) const { return addr <= rhs.addr; }
bool PyModule::operator== (const PyModule rhs) const { return addr == rhs.addr; }
#endif // PyModule methods

static std::list<PyModule*> PyModObjs;  //  Python objects, mudules...
static std::list<PyVar*> PyVarObjs;     //  Python variables, numeric and string
MSEXPORT void AddToPyList(PyVar* V) { PyVarObjs.push_back(V); }
MSEXPORT void AddToPyMods(PyModule* V) { PyModObjs.push_back(V); }

struct {bool err = false; string* str = NULL;} ObjErr; //  where we store user-checked/non-API error info

bool IsPyVar(PyVar* addr) //  check for corruption/validity, use <list> to track all open control/monitor objects, avoid SEGFAULT
{
    if (addr == NULL) 
        {ObjErr.str = new string("NULL Python Object"); ObjErr.err = true; return false; }
    bool b = false;
    for (auto i : PyVarObjs) { if (i->addr == addr) {b = true; break;}}
    if (!b) 
        {ObjErr.str = new string("Invalid Python object (unallocated memory or non-control reference)"); ObjErr.err = true; return false; }

    if (addr->addr == addr && addr->canary_end == MAGIC)
        { ObjErr.err = false; delete ObjErr.str; return true; }
    else { ObjErr.err = true; ObjErr.str = new string("Control/monitor memory corrupted"); return false; }
}

extern "C" {  //  functions to be called from LabVIEW.  'extern "C"' is necessary to prevent overload name mangling

#define LStrString(A) string((char*) (*A)->str, (*A)->cnt)
MSEXPORT int PyInit() { //  initialize Python interpreter
    if (sys == NULL)
    {
        Py_Initialize();
        sys = PyImport_AddModule("sys");
        module = PyImport_AddModule("__main__");
        if (setup_stderr() == 0) return -1;
    }
    return 0;
}

MSEXPORT int PyRun(LStrHandle cmd, tLvVarErr* error) {  //  run simple Python program (no parameter exchange)
    int rc = PyRun_SimpleString(LStrString(cmd).c_str());
    if (rc)
    {
        error->errnum = rc;
        if (LStrString(cmd).size() > 0) error->errdata = LVStr(LStrString(cmd));
        LStrHandle errstr = get_stderr_text();
        if (errstr != NULL) error->errstr = errstr;
        else error->errstr = LVStr("Unknown error");
    }
    return rc;
}

MSEXPORT PyVar* PyVarCreate(VarObj* data, tLvVarErr* error) {
    if (sys == NULL)    //  no place to set the name attribute
        {ObjErr.err = true; ObjErr.str = new string("Python interpreter not initialized"); return NULL;}   
    if (!IsVariant(data)) { //  check for valid Variant, on error, get object error
        tObjErr* VarErr = (tObjErr*) GetObjErr();
        ObjErr.err = VarErr->err; ObjErr.str = VarErr->str;
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

MSEXPORT PyModule* PyModCreate(char* data, tLvVarErr* error) {
    if (sys == NULL)    //  no place to set the name attribute
        {error->errnum = -1; error->errstr = LVStr("Python interpreter not initialized"); return NULL;}
    if (strlen(data) == 0) { //  check for valid name
        error->errnum = -1; error->errstr = LVStr("module name may not be blank"); return NULL;}

    PyModule* pValue = new PyModule(data);
    if (pValue->errnum) {
        error->errnum = pValue->errnum;
        if (pValue->errdata != NULL) error->errdata = LVStr(*(pValue->errdata));
        if (pValue->errstr != NULL) error->errstr = LVStr(*(pValue->errstr));
        else error->errstr = LVStr("Unknown error");
    }
    else AddToPyMods(pValue);   //  SUCCESS, add to list to track.
    return pValue;
}

MSEXPORT int PyVarDestroy(PyVar* var) {
    if (!IsPyVar(var)) return -1;  //  don't want to risk a dump in case of invalid data
    else delete var;
    return 0;
}

#if 1   //  utility-ish functions

MSEXPORT void GetError(PyVar* CtrlObj, tLvVarErr* error) { //  get error info
    if (CtrlObj == NULL || ObjErr.err) { //  NULL object, or error in object creation
        if (!ObjErr.err) return; //  no error
        error->errnum = -1;
        error->errstr = LVStr(*ObjErr.str);
        ObjErr.err = false; delete ObjErr.str; //  Clear error, but race conditions may exist, if so, da shit has hit da fan. 
    }
    else
    {   //  API error info is stored in object, pass up to caller
        error->errnum = CtrlObj->errnum;
        if (CtrlObj->errdata != NULL) error->errdata = LVStr(*(CtrlObj->errdata));
        if (CtrlObj->errstr != NULL) error->errstr = LVStr(*(CtrlObj->errstr));
        else error->errstr = LVStr("Unknown error");
        CtrlObj->errnum = 0;
        delete CtrlObj->errstr;  CtrlObj->errstr  = NULL;
        delete CtrlObj->errdata; CtrlObj->errdata = NULL;
    }
}

MSEXPORT void Version(char* buf, int sz) {   //  get version string
    memcpy(buf, string(PYVAR_VERSION).c_str(), min<int>(sz, string(PYVAR_VERSION).length()));}

#endif //   end utility-ish functions

}   //  end extern "C"