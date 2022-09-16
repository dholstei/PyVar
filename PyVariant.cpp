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

#if 1 // PyVar methods

PyVar::PyVar(string* n, LvVariant* d) {
    if (d == NULL)
        {errno = -1; errstr = new string("NULL object passed");}
    addr = this;}
PyVar::~PyVar() {
    delete errstr;  errstr  = NULL;
    delete errdata; errdata = NULL;
}

bool PyVar::operator<  (const PyVar rhs) const { return addr <  rhs.addr; }
bool PyVar::operator<= (const PyVar rhs) const { return addr <= rhs.addr; }
bool PyVar::operator== (const PyVar rhs) const { return addr == rhs.addr; }

#endif // PyVar methods

static std::list<PyVar*> PyVarObjs;

#if 1 // Python stuff
static PyObject*sys = NULL, *io = NULL, *stringio = NULL, 
                *stringioinstance = NULL, * module = NULL;
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

#ifdef WIN32
#define MSEXPORT __declspec(dllexport)
#else
#define MSEXPORT
#endif // WIN32

extern "C" {  //  functions to be called from LabVIEW.  'extern "C"' is necessary to prevent overload name mangling

#define LStrString(A) string((char*) (*A)->str, (*A)->cnt)
MSEXPORT int PyInit() { //  initialize Python interpreter
    if (sys == NULL)
    {
        Py_Initialize();
        sys = PyImport_ImportModuleLevel("sys", NULL, NULL, NULL, 0);
        //module = PyImport_ImportModuleLevel("__main__", NULL, NULL, NULL, 0);
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

enum VarIdx :int { U8 = 1, I32 = 4, U32 = 5, DBL = 7, Str = 8 };
MSEXPORT PyObject* PyVar(VarObj* data, tLvVarErr* error) {
    PyObject* module_name, * dict, * python_class, * object;
    if (sys == NULL)
        {ObjErr.err = true; ObjErr.str = new string("Python interpreter not initialized"); return NULL;}   
    if (!IsVariant(data)) { //  check for valid Variant, on error, get object error
        tObjErr* VarErr = (tObjErr*) GetObjErr();
        ObjErr.err = VarErr->err; ObjErr.str = VarErr->str;
        return NULL;}

    PyObject *pValue = NULL;
    switch (data->data.index())
    {
    case VarIdx::I32:
        pValue = PyLong_FromLong((long) get<int32_t>(data->data));
        break;
    case VarIdx::DBL:
        pValue = PyFloat_FromDouble ((double) get<double>(data->data));
        break;
    case VarIdx::Str :
        pValue = PyUnicode_FromString((char*) get<string*>(data->data)->c_str());
        break;
    default:
        error->errnum = -1; error->errdata = LVStr(*(data->name));
        error->errstr = LVStr(string("Unsupported data type"));
        return NULL;
    }
    int rc = PyObject_SetAttrString(sys, data->name->c_str(), pValue);

    return pValue;
}

MSEXPORT void* Variant(PyObject* pValue) {
    PyTypeObject* type = Py_TYPE(pValue);
    if (type->tp_name == string("int"))
    {
        int i = PyLong_AsLong(pValue);
        PyObject* dict = sys->ob_type->tp_dict;
        PyObject* Vals = PyDict_Keys(dict);
        if (Vals == NULL) return NULL;
        for (size_t i = 0; i < PyDict_Size(dict); i++)
        {
            PyObject* it = PyList_GetItem(Vals, i);
                //MessageBoxA(NULL, PyUnicode_AsUTF8(it), "debug", 0);
            if (string("str") == string(it->ob_type->tp_name))
            {
                //MessageBoxA(NULL, (char*) PyUnicode_AsASCIIString(it), "debug", 0);
                i = i;

            }
        }
        i++;
    }
    return NULL;
}

#if 1   //  utility-ish functions

//MSEXPORT void GetError(PyVar* CtrlObj, tLvVarErr* error) { //  get error info
//    if (CtrlObj == NULL || ObjErr.err) { //  NULL object, or error in object creation
//        if (!ObjErr.err) return; //  no error
//        error->errnum = -1;
//        error->errstr = LVStr(*ObjErr.str);
//        ObjErr.err = false; delete ObjErr.str; //  Clear error, but race conditions may exist, if so, da shit has hit da fan. 
//    }
//    else
//    {   //  API error info is stored in object, pass up to caller
//        error->errnum = CtrlObj->errnum;
//        if (CtrlObj->errdata != NULL) error->errdata = LVStr(*(CtrlObj->errdata));
//        if (CtrlObj->errstr != NULL) error->errstr = LVStr(*(CtrlObj->errstr));
//        else error->errstr = LVStr("Unknown error");
//        CtrlObj->errnum = 0;
//        delete CtrlObj->errstr;  CtrlObj->errstr  = NULL;
//        delete CtrlObj->errdata; CtrlObj->errdata = NULL;
//    }
//}

MSEXPORT void Version(char* buf, int sz) {   //  get version string
    memcpy(buf, string(PYVAR_VERSION).c_str(), min<int>(sz, string(PYVAR_VERSION).length()));}

#endif //   end utility-ish functions

}   //  end extern "C"