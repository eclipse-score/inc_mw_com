
import struct

def version_display(valobj, internal_dict):
    value = valobj.GetChildMemberWithName('version').GetValueAsUnsigned(0)

    major = (value >> 54) & 1023
    minor = (value >> 44) & 1023
    patch = (value >> 34) & 1023
    build = value & ((1<<32)-1)
  
    return f'{major}.{minor}.{patch} {build}'

def id_display(valobj, internal_dict):
    value = valobj.GetChildMemberWithName('index').GetValueAsUnsigned(0)
  
    return f'#{value:03d}' if value != 0xffffffffffffffff else '#invalid'

def tag_display(valobj, internal_dict):
    value = valobj.GetChildMemberWithName('tag').GetValueAsUnsigned(0)

    bytes = struct.pack('<Q', value)
    if all(32 <= b <= 127 for b in bytes):
        str = ''.join(chr(ch) for ch in bytes)
        return f"#{str} 0x{value:16x}"
    
    return f'#{value:016x}' if value != 0xffffffffffffffff else '#invalid'


def __lldb_init_module(debugger, internal_dict):
    print("Loading debug symbols for 'qor_core'")

    debugger.HandleCommand('type summary add -F qor_core_debug.version_display "qor_core::Version"')
    debugger.HandleCommand('type summary add -F qor_core_debug.id_display "qor_core::id::Id"')
    debugger.HandleCommand('type summary add -F qor_core_debug.tag_display "qor_core::id::Tag"')                           

if __name__ == '__main__':
    pass