
import struct

def __lldb_init_module(debugger, internal_dict):
    print("Loading debug symbols for 'qor_com'")

    #debugger.HandleCommand('type summary add -F qor_core_debug.version_display "qor_core::Version"')                   

if __name__ == '__main__':
    pass