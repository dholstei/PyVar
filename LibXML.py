# libxml2 is a software library for parsing XML documents.
'''
This lib is used instead of the standard Python XML lib because it's using the regular (or user-specified) libxml2 DLL, 
which means documents and nodes can be shared with C/C++ code/objects directly.
'''

import ctypes
import sys
from PyQt5.QtWidgets import (
    QApplication, 
    QFileDialog
)
from numpy import array
# import MessageBox as M
import utils

class PointerPtr(ctypes.Structure):
    _fields_ = [
        ("ptr",ctypes.c_void_p),        #    pointer
    ]

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
        ("type",ctypes.c_int16),        #    type number, must be second !
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

class xmlXPathParserContext(ctypes.Structure):
    _fields_ = [
        ("cur",ctypes.c_char_p),        #    the current char being parsed
        ("base",ctypes.c_void_p),       #    the full expression
        ("error",ctypes.c_int),         #    error code
        ("context",ctypes.c_char_p),    #    the evaluation context
        ("value",ctypes.c_char_p),      #    the current value
        ("valueNr",ctypes.c_int),       #    number of values stacked
        ("valueMax",ctypes.c_int),      #    max number of values stacked
        ("valueTab",ctypes.c_char_p),   #    stack of values
        ("comp",ctypes.c_char_p),       #    the precompiled expression
        ("xptr",ctypes.c_int),          #    it this an XPointer expression
        ("ancestor",ctypes.c_char_p),   #    used for walking preceding axis
        ("valueFrame",ctypes.c_int),    #    used to limit Pop on the stack
    ]

class xmlXPathObject(ctypes.Structure):
    _fields_ = [
        ("type",ctypes.c_char_p), 
        ("nodesetval",ctypes.c_void_p), 
        ("boolval",ctypes.c_int), 
        ("floatval",ctypes.c_double), 
        ("stringval",ctypes.c_void_p), 
        ("user",ctypes.c_void_p), 
        ("index",ctypes.c_int), 
        ("user2",ctypes.c_void_p), 
        ("index2",ctypes.c_int), 
    ]

class xmlNodeSet(ctypes.Structure):
    _fields_ = [
        ("nodeNr",ctypes.c_int),        #    number of nodes in the set
        ("nodeMax",ctypes.c_int),       #    size of the array as allocated
        ("nodeTab",ctypes.c_void_p),    #    array of nodes in no particular order @@ with_ns to check whether nam
    ]

if utils.os == 'Linux':
    if utils.x64: libXmlPath = "/usr/lib64/libxml2.so.2"
    else: libXmlPath = "/usr/lib/libxml2.so.2"
else:
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

class xmlNullPtr(Exception):
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
        err -- XML error structure
    """

    def __init__(self, err=None):
        if err == None:
            self.message = "undefined libxml2 error"
        else:
            e = err.contents
            self.message = e.file.decode("utf-8") + " line:" + str(e.line) + " msg:" + e.message.decode("utf-8")
        super().__init__(self.message)

def xmlGetLastError () -> xmlError:
# Get the last global error registered. This is per thread if compiled with thread support.
# Returns:	NULL if no error occurred or a pointer to the error

    global libXML
# Check if DLL into memory. 
    if libXML == None: raise NullDLL()

# set up prototype
    libXML.xmlGetLastError.restype = ctypes.POINTER(xmlError) # correct return type

# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    ErrPtr = libXML.xmlGetLastError()
    if ErrPtr: return ErrPtr
    else: return None

def SetLibXmlPath(path: str):
    global libXmlPath
    libXmlPath = path

def xmlReadFile (filename: str, encoding: str, options) -> xmlDoc:
# parse an XML file from the filesystem or the network.
#         filename:	a file or URL
#         encoding:	the document encoding, or NULL
#         options:	a combination of xmlParserOption
#         Returns:	the resulting document tree
    
    global libXML
# Load DLL into memory. 
    if libXML == None:
        global libXmlPath
        if utils.os == 'Linux': libXML = ctypes.CDLL (libXmlPath)
        else: libXML = ctypes.WinDLL (libXmlPath)

# set up prototype
    libXML.xmlReadFile.restype = ctypes.POINTER(xmlDoc) # correct return type
    libXML.xmlReadFile.argtypes = ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int32

# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    pDoc = libXML.xmlReadFile(filename.encode(), encoding.encode(), options)
    Err = xmlGetLastError()
    if Err: raise LibErr(Err)
    return pDoc # return xmlDocPtr (xmlDoc*). Add ".contents" in calling program to dereference.

def xmlReadMemory (XML: str, URL: str, encoding: str, options) -> xmlDoc:
# parse an XML in-memory document and build a tree.
#     buffer:   a pointer to a char array
#     size:	    the size of the array
#     URL:	    the base URL to use for the document
#     encoding:	the document encoding, or NULL
#     options:	a combination of xmlParserOption
#     Returns:	the resulting document tree
    
    global libXML
# Load DLL into memory. 
    if libXML == None:
        global libXmlPath
        if utils.os == 'Linux': libXML = ctypes.CDLL (libXmlPath)
        else: libXML = ctypes.WinDLL (libXmlPath)

# set up prototype
    libXML.xmlReadMemory.restype = ctypes.POINTER(xmlDoc) # correct return type
    libXML.xmlReadMemory.argtypes = ctypes.c_char_p, ctypes.c_int32, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int32
  
# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    pDoc = libXML.xmlReadMemory(XML.encode(), len(XML), URL.encode(), encoding.encode(), options)
    Err = xmlGetLastError()
    if Err: raise LibErr(Err)
    return pDoc # return xmlDocPtr (xmlDoc*). Add ".contents" in calling program to dereference.

def xmlFreeDoc (cur: ctypes.c_void_p) :
# Free up all the structures used by a document, tree included.
#     cur:	pointer to the document

# set up prototype
    libXML.xmlFreeDoc.argtypes = ctypes.c_void_p,
  
# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    libXML.xmlFreeDoc(cur)
    Err = xmlGetLastError()
    if Err: raise LibErr(Err)
    return

def xmlDocGetRootElement (doc: xmlDoc) -> xmlNode:
# Get the root element of the document (doc->children is a list containing possibly comments, PIs, etc ...).
#     doc:	the document
#     Returns:	the #xmlNodePtr for the root or NULL

    if doc == None: raise xmlNullPtr("XML Document pointer may not be NULL")
    global libXML
# Check if DLL into memory. 
    if libXML == None: raise NullDLL()

# set up prototype
    libXML.xmlDocGetRootElement.restype = ctypes.POINTER(xmlNode) # correct return type
    libXML.xmlDocGetRootElement.argtypes = ctypes.c_void_p,

# wrapping in c_char_p or c_int32 isn't required because .argtypes are known.
    NodePtr = libXML.xmlDocGetRootElement(doc)
    return NodePtr   #   return xmlNodePtr (xmlNode*). Add ".contents" in calling program to dereference.

def xmlXPathNewContext (doc: xmlDoc) -> xmlXPathParserContext:
# Create a new xmlXPathContext
#     doc:	the XML document
#     Returns:	the xmlXPathContext just allocated. The caller will need to free it.

    if doc == None: raise xmlNullPtr("XML Document pointer may not be NULL")
    global libXML

# Check if DLL loaded. 
    if libXML == None: raise NullDLL()

# set up prototype
    libXML.xmlXPathNewContext.restype = ctypes.POINTER(xmlXPathParserContext) # correct return type
    libXML.xmlXPathNewContext.argtypes = ctypes.c_void_p,
    return libXML.xmlXPathNewContext(doc)
    
def xmlXPathFreeContext (ctxt: xmlXPathParserContext) :
# Free up an xmlXPathContext
#     ctxt:	the context to free

# set up prototype
    libXML.xmlXPathFreeContext.argtypes = ctypes.c_void_p,
    libXML.xmlXPathFreeContext(ctxt)
    Err = xmlGetLastError()
    if Err: raise LibErr(Err)
    return

def xmlXPathEval (str: str, ctx: xmlXPathParserContext) -> xmlXPathObject:
# Evaluate the XPath Location Path in the given context.
#     str:	the XPath expression
#     ctx:	the XPath context
#     Returns:	the xmlXPathObjectPtr resulting from the evaluation or NULL. the caller has to free the object.

#   Note: The reason I don't open/close a context ptr for each XPath is you may want to keep it around for multiple queries, or not
    if ctx == None: raise xmlNullPtr("XPath context pointer may not be NULL")
    global libXML
# Check if DLL is loaded. 
    if libXML == None: raise NullDLL()

# set up prototype
    libXML.xmlXPathEval.restype = ctypes.POINTER(xmlXPathObject) # correct return type
    libXML.xmlXPathEval.argtypes = ctypes.c_char_p, ctypes.c_void_p,
    return libXML.xmlXPathEval(str.encode() , ctx)

def xmlNodeArray(xmlXPathObj: xmlXPathObject) -> array:
    if not xmlXPathObj: return None # null XPath results
    x = xmlXPathObj.contents.nodesetval
    if not x: return None           # no nodeset
    ns = ctypes.cast(x, ctypes.POINTER(xmlNodeSet)).contents
    if ns.nodeNr == 0: return None  # empty nodeset
    n = []
    xmlNodePtr = ctypes.POINTER(xmlNode)
    xmlNodePtrPtr = ctypes.POINTER(xmlNodePtr)
    for i in range(ns.nodeNr):
        p = ctypes.cast(ns.nodeTab + i*ctypes.sizeof(xmlNodePtr), 
                            xmlNodePtrPtr)
        print(p.contents.contents.name)
        n.append(p.contents.contents)   #   dereference twice since pointer to pointer
    return n   # return python array of NodeSet 

def test():
    app = QApplication(sys.argv)
    FileObj = QFileDialog.getOpenFileName(None, "Select XML file to load", None, "XML (*.xml)")

    if len(FileObj[0]) > 0:
        pDoc = xmlReadFile(FileObj[0], "UTF-8", 0)
        ctypes.c_double
        if True:
            # M.MessageBox(0, f"{pDoc:#0{10}x}", "XML doc ptr", 0)
            D = pDoc.contents
            print(D.encoding)
        pNode = xmlDocGetRootElement(pDoc)
        root = pNode.contents
        print (root.name)
    else:
        s = """
 <notice>
  <to>Tove</to>
  <from>Jani</from>
  <heading>Reminder</heading>
  <body>Don't forget me this weekend!</body>
</notice>   """
        pDoc = xmlReadMemory(s, "", "UTF-8", 0)
        
        if True:
            # M.MessageBox(0, f"{pDoc:#0{10}x}", "XML doc ptr", 0)
            D = pDoc.contents
            print(D.encoding)
        pNode = xmlDocGetRootElement(pDoc)
        root = pNode.contents
        print (root.name)
    
    ctxt = xmlXPathNewContext(pDoc)
    # XPathObj = xmlXPathEval("count(/*/*)", ctxt)
    # print (int(XPathObj.contents.floatval))
    XPathObj = xmlXPathEval("/*/*", ctxt)
    NodeSet = xmlNodeArray(XPathObj)
    for n in NodeSet:
        print(n.name)
    xmlXPathFreeContext(ctxt)
    if pDoc : xmlFreeDoc(pDoc)

    return

test()