# libxml2 is a software library for parsing XML documents.
'''
This lib is used instead of the standard Python XML lib because it's using the regular libxml2 DLL, 
which means documents and nodes can be shared with C++ objects directly.
'''

import ctypes
import sys
from PyQt5.QtWidgets import (
    QApplication, 
    QFileDialog
)
import MessageBox as M

def xmlReadFile (filename, encoding, options):
# parse an XML file from the filesystem or the network.

#         filename:	a file or URL
#         encoding:	the document encoding, or NULL
#         options:	a combination of xmlParserOption
#         Returns:	the resulting document tree

# Load DLL into memory. 
    X = ctypes.WinDLL ("C:\\Users\\dholstein\\Documents\\XML\\lib\\libxml2.dll")

# set up prototype, first param is return value
    XApiProto = ctypes.WINFUNCTYPE (ctypes.c_uint32, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int32)
    XApiParams = (1, "filname_", 0), (1, "encoding_", 0),  (1, "options_", 0),
    XApi = XApiProto (("xmlReadFile", X), XApiParams)   #   DLL function name

#   data converted to ctypes and call    
    DocPtr = XApi ( ctypes.create_string_buffer (filename.encode('utf-8')), 
                ctypes.create_string_buffer (encoding.encode('utf-8')), 
                ctypes.c_int32 (options))
    return DocPtr

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

def test():
    app = QApplication(sys.argv)
    FileObj = QFileDialog.getOpenFileName(None, "Select XML file to load", None, "XML (*.xml)")

    if len(FileObj[0]) > 0:
        pDoc = xmlReadFile(FileObj[0], "UTF-8", 0)
        # M.MessageBox(0, f"{pDoc:#0{10}x}", "XML doc ptr", 0)
        D = ctypes.POINTER(xmlDoc)
        ctypes.cast(pDoc, D)
        print(D.__dict__.get("standalone"))

    M.MessageBox(0, "fin", "Caption-y stuff", 0)
    return

test()