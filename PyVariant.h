#pragma once

#ifdef WIN32
#define MSEXPORT __declspec(dllexport)
#else
#define MSEXPORT
#endif // WIN32

#include <list>     //  container of generated objects for error checking (avoid SEGFAULT)
#include <variant>  //  container for variant data
#include <iostream> //  std::cout
#include <inttypes.h>
#include <memory.h>
#include "LvVariant++.h"
#include "extcode.h"

#define MAGIC 0x13131313    //  random, unique, non 0x00000000 and 0xffffffff number
#define VARTYPE int, double, string*, uint8_t*, bool

class PyVar
{
public:
    void* addr;
    PyObject* PyObj = NULL;
    string* name = NULL;
    int errnum = 0; string* errstr = NULL, * errdata = NULL;

    PyVar(VarObj* d);
    ~PyVar();


    bool operator<  (const PyVar rhs) const;
    bool operator<= (const PyVar rhs) const;
    bool operator== (const PyVar rhs) const;
    uint32_t canary_end = MAGIC;  //  check for buffer overrun/corruption
private:

};

class PyModule
{
public:
    void* addr;
    PyObject* PyObj = NULL;
    string* name = NULL;
    int errnum = 0; string* errstr = NULL, * errdata = NULL;

    PyModule(char* d);
    ~PyModule();


    bool operator<  (const PyModule rhs) const;
    bool operator<= (const PyModule rhs) const;
    bool operator== (const PyModule rhs) const;
    uint32_t canary_end = MAGIC;  //  check for buffer overrun/corruption
private:

};

enum VarIdx :int { U8 = 1, I32 = 4, U32 = 5, DBL = 7, Str = 8 };