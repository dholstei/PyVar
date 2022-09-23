
# MessageBox(0, str("Stuff to display"), str("Caption-y stuff"), 0)
# MessageBox(0, "Stuff to display", "Caption-y stuff", 0)

def MessageBox (hWnd, lpText, lpCaption, uType):
    import os
    import ctypes
    
# Load DLL into memory. 
    MsgBox = ctypes.WinDLL ("User32.dll")

# set up prototype, first param is return value
    MsgBoxApiProto = ctypes.WINFUNCTYPE (ctypes.c_int, ctypes.c_uint32, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint32)
    MsgBoxApiParams = (1, "hWnd_", 0), (1, "lpText_", 0),  (1, "lpCaption_", 0),  (1, "uType_", 0), 
    MsgBoxApi = MsgBoxApiProto (("MessageBoxA", MsgBox), MsgBoxApiParams)   #   DLL function name

#   data converted to ctypes and call    
    rc = MsgBoxApi (ctypes.c_uint32 (hWnd),
                    ctypes.create_string_buffer (lpText.encode('utf-8')), 
                    ctypes.create_string_buffer (lpCaption.encode('utf-8')), 
                    ctypes.c_uint32 (uType))
    return rc
