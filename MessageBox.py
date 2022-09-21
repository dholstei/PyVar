def MessageBox (hWnd, lpText, lpCaption, uType):
    import os
    import ctypes
    
# Load DLL into memory. 
    MsgBox = ctypes.WinDLL ("User32.dll")

# set up prototype, first param is return value
    MsgBoxApiProto = ctypes.WINFUNCTYPE (ctypes.c_int, ctypes.c_uint32, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint32)
    MsgBoxApiParams = (1, "hWnd_", 0), (1, "lpText_", 0),  (1, "lpCaption_", 0),  (1, "uType_", 0), 
    MsgBoxApi = MsgBoxApiProto (("MessageBoxA", MsgBox), MsgBoxApiParams)   #   DLL function name

#   data converted to ctypes
    hWnd_ = ctypes.c_uint32 (hWnd)
    lpText_ = ctypes.create_string_buffer (lpText)
    lpCaption_ = ctypes.create_string_buffer (lpCaption)
    uType_ = ctypes.c_uint32 (uType)
    
    rc = MsgBoxApi (hWnd_, lpText_, lpCaption_, uType_) # actual call
    return rc

# MessageBox(0, b"Stuff to display", b"Caption-y stuff", 0)
