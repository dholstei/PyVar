# libxml2 is a software library for parsing XML documents.
'''
This lib is used instead of the standard Python XML lib because it's using the regular (or user-specified) libxml2 DLL, 
which means documents and nodes can be shared with C/C++ code/objects directly.
'''

from asyncio.windows_events import NULL
import ctypes
import sys
from PyQt5.QtWidgets import (
    QApplication, 
    QFileDialog
)
from pyparsing import line
import MessageBox as M

class xmlDoc(ctypes.Structure):
    _fields_ = [
        ("_private",ctypes.c_void_p),   #    application data
        ("type",ctypes.c_uint16),       #    XML_DOCUMENT_NODE, must be second !
        ("name",ctypes.c_char_p),       #    name/filename/URI of the document
        ("children",ctypes.c_void_p),   #    the document tree
        ("last",ctypes.c_void_p),       #    last child link
        ("parent",ctypes.c_void_p),     #    child->parent link
        ("next",ctypes.c_void_p),       #    next sibling link
        ("prev",ctypes.c_void_p),       #    previous sibling link
        ("doc",ctypes.c_void_p),        #    autoreference to itself End of common part
        ("compression",ctypes.c_int),   #    level of zlib compression
        ("standalone",ctypes.c_int),    #    standalone document (no external refs) 1 if standalone="yes" 0 if sta
        ("intSubset",ctypes.c_void_p),  #    the document internal subset
        ("extSubset",ctypes.c_void_p),  #    the document external subset
        ("oldNs",ctypes.c_void_p),      #    Global namespace, the old way
        ("version",ctypes.c_char_p),    #    the XML version string
        ("encoding",ctypes.c_char_p),   #    external initial encoding, if any
        ("ids",ctypes.c_void_p),        #    Hash table for ID attributes if any
        ("refs",ctypes.c_void_p),       #    Hash table for IDREFs attributes if any
        ("URL",ctypes.c_char_p),        #    The URI for that document
        ("charset",ctypes.c_int),       #    Internal flag for charset handling, actually an xmlCharEncoding
        ("dict",ctypes.c_void_p),       #    dict used to allocate names or NULL
        ("psvi",ctypes.c_void_p),       #    for type/PSVI information
        ("parseFlags",ctypes.c_int),    #    set of xmlParserOption used to parse the document
        ("properties",ctypes.c_int),    #    set of xmlDocProperties for this document set at the end of parsing
    ]

class xmlNode(ctypes.Structure):
    _fields_ = [
        ("_private",ctypes.c_void_p),   #    application data
        ("type",ctypes.c_uint16),       #    type number, must be second !
        ("name",ctypes.c_char_p),       #    the name of the node, or the entity
        ("children",ctypes.c_void_p),   #    parent->childs link
        ("last",ctypes.c_void_p),       #    last child link
        ("parent",ctypes.c_void_p),     #    child->parent link
        ("next",ctypes.c_void_p),       #    next sibling link
        ("prev",ctypes.c_void_p),       #    previous sibling link
        ("doc",ctypes.c_void_p),        #    the containing document End of common part
        ("ns",ctypes.c_void_p),         #    pointer to the associated namespace
        ("content",ctypes.c_void_p),    #    the content
        ("properties",ctypes.c_char_p), #    properties list
        ("nsDef",ctypes.c_char_p),      #    namespace definitions on this node
        ("psvi",ctypes.c_void_p),       #    for type/PSVI information
        ("line",ctypes.c_uint16),       #    line number
        ("extra",ctypes.c_uint16),      #    extra data for XPath/XSLT
    ]

class xmlError(ctypes.Structure):
    _fields_ = [
        ("domain",ctypes.c_int),        #    What part of the library raised this error
        ("code",ctypes.c_int),          #    The error code, e.g. an xmlParserError
        ("message",ctypes.c_char_p),    #    human-readable informative error message
        ("level",ctypes.c_uint32),      #    how consequent is the error
        ("file",ctypes.c_char_p),       #    the filename
        ("line",ctypes.c_int),          #    the line number if available
        ("str1",ctypes.c_char_p),       #    extra string information
        ("str2",ctypes.c_char_p),       #    extra string information
        ("str3",ctypes.c_char_p),       #    extra string information
        ("int1",ctypes.c_int),          #    extra number information
        ("int2",ctypes.c_int),          #    error column # or 0 if N/A (todo: rename field when we would brk ABI)
        ("ctxt",ctypes.c_void_p),       #    the parser context if available
        ("node",ctypes.c_void_p),       #    the node in the tree
    ]

libXmlPath = "C:\\Users\\dholstein\\Documents\\XML\\lib\\libxml2.dll"
libXML = None
class NullDLL(Exception):
    """Exception raised if "libXML = None"

    Attributes:
        message -- explanation of the error
    """

    def __init__(self, message="DLL not loaded"):
        self.message = message
        super().__init__(self.message)

class NullDocPtr(Exception):
    """Exception raised if "xmlDocPtr = NULL"

    Attributes:
        message -- explanation of the error
    """

    def __init__(self, message="XML Document pointer may not be NULL"):
        self.message = message
        super().__init__(self.message)

class LibErr(Exception):
    """Exception raised if xmlError is not NULL

    Attributes:
        message -- explanation of the error
    """

    def __init__(self, err=None):
        if err == None:
            self.message = "undefined libxml2 error"
        else:
            self.message = err.contents.file + ":" + line + " msg:" + err.contents.message
        super().__init__(self.message)

def xmlGetLastError ():
# Get the last global error registered. This is per thread if compiled with thread support.
# Returns:	NULL if no error occurred or a pointer to the error

    global libXML
# Check if DLL into memory. 
    if libXML == None:
        raise NullDLL()

# set up prototype
    libXML.xmlGetLastError.restype = ctypes.POINTER(xmlError) # correct return type

# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    ErrPtr = libXML.xmlGetLastError()
    if ErrPtr.contents == NULL:
        return None
    else:
        return ErrPtr

def SetLibXmlPath(path):
    global libXmlPath
    libXmlPath = path

def xmlReadFile (filename, encoding, options):
# parse an XML file from the filesystem or the network.
#         filename:	a file or URL
#         encoding:	the document encoding, or NULL
#         options:	a combination of xmlParserOption
#         Returns:	the resulting document tree
    
    global libXML
# Load DLL into memory. 
    if libXML == None:
        global libXmlPath
        libXML = ctypes.WinDLL (libXmlPath)

# set up prototype
    libXML.xmlReadFile.restype = ctypes.POINTER(xmlDoc) # correct return type
    libXML.xmlReadFile.argtypes = ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int32

# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    pDoc = libXML.xmlReadFile(filename.encode(), encoding.encode(), options)
    Err = xmlGetLastError()
    return pDoc # return xmlDocPtr (xmlDoc*). Add ".contents" in calling program to dereference.

def xmlDocGetRootElement (doc):
# Get the root element of the document (doc->children is a list containing possibly comments, PIs, etc ...).
#     doc:	the document
#     Returns:	the #xmlNodePtr for the root or NULL

    if doc == None:
        raise NullDocPtr()
    global libXML
# Check if DLL into memory. 
    if libXML == None:
        raise NullDLL()

# set up prototype
    libXML.xmlDocGetRootElement.restype = ctypes.POINTER(xmlNode) # correct return type
    libXML.xmlDocGetRootElement.argtypes = ctypes.c_void_p,

# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    NodePtr = libXML.xmlDocGetRootElement(doc)
    return NodePtr   #   return xmlNodePtr (xmlNode*). Add ".contents" in calling program to dereference.

def test():
    app = QApplication(sys.argv)
    FileObj = QFileDialog.getOpenFileName(None, "Select XML file to load", None, "XML (*.xml)")

    if len(FileObj[0]) > 0:
        pDoc = xmlReadFile(FileObj[0], "UTF-8", 0)
        
        if True:
            # M.MessageBox(0, f"{pDoc:#0{10}x}", "XML doc ptr", 0)
            D = pDoc.contents
            print(D.encoding)
        pNode = xmlDocGetRootElement(pDoc)
        root = pNode.contents
        print (root.name)


    # M.MessageBox(0, "fin", "Caption-y stuff", 0)
    return

test()