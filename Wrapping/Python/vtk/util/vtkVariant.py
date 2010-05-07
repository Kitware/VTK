"""
Utility functions to mimic the template support functions for vtkVariant
"""

import vtk
import vtkConstants

_variant_type_map = {
    'char' : vtkConstants.VTK_CHAR,
    'unsigned char' : vtkConstants.VTK_UNSIGNED_CHAR,
    'signed char' : vtkConstants.VTK_SIGNED_CHAR,
    'short' : vtkConstants.VTK_SHORT,
    'unsigned short' : vtkConstants.VTK_UNSIGNED_SHORT,
    'int' : vtkConstants.VTK_INT,
    'unsigned int' : vtkConstants.VTK_UNSIGNED_INT,
    'long' : vtkConstants.VTK_LONG,
    'unsigned long' : vtkConstants.VTK_UNSIGNED_LONG,
    'long long' : vtkConstants.VTK_LONG_LONG,
    'unsigned long long' : vtkConstants.VTK_UNSIGNED_LONG_LONG,
    '__int64' : vtkConstants.VTK___INT64,
    'unsigned __int64' : vtkConstants.VTK_UNSIGNED___INT64,
    'float' : vtkConstants.VTK_FLOAT,
    'double' : vtkConstants.VTK_DOUBLE,
    'string' : vtkConstants.VTK_STRING,
    'unicode string' : vtkConstants.VTK_UNICODE_STRING,
}

_variant_method_map = {
    'char' : 'ToChar',
    'unsigned char' : 'ToUnsignedChar',
    'signed char' : 'ToSignedChar',
    'short' : 'ToShort',
    'unsigned short' : 'ToUnsignedShort',
    'int' : 'ToInt',
    'unsigned int' : 'ToUnsignedInt',
    'long' : 'ToLong',
    'unsigned long' : 'ToUnsignedLong',
    'long long' : 'ToLongLong',
    'unsigned long long' : 'ToUnsignedLongLong',
    '__int64' : 'To__Int64',
    'unsigned __int64' : 'ToUnsigned__Int64',
    'float' : 'ToFloat',
    'double' : 'ToDouble',
    'string' : 'ToString',
    'unicode string' : 'ToUnicodeString',
}

_variant_check_map = {
    'char' : 'IsChar',
    'unsigned char' : 'IsUnsignedChar',
    'signed char' : 'IsSignedChar',
    'short' : 'IsShort',
    'unsigned short' : 'IsUnsignedShort',
    'int' : 'IsInt',
    'unsigned int' : 'IsUnsignedInt',
    'long' : 'IsLong',
    'unsigned long' : 'IsUnsignedLong',
    'long long' : 'IsLongLong',
    'unsigned long long' : 'IsUnsignedLongLong',
    '__int64' : 'Is__Int64',
    'unsigned __int64' : 'IsUnsigned__Int64',
    'float' : 'IsFloat',
    'double' : 'IsDouble',
    'string' : 'IsString',
    'unicode string' : 'IsUnicodeString',
}


def vtkVariantCreate(v, t):
    """
    Create a vtkVariant of the specified type, where the type is in the
    following format: 'int', 'unsigned int', etc. for numeric types,
    'string', and 'unicode string'.
    """
    try:
        i = _variant_type_map[t]
    except KeyError:
        raise ValueError, "second parameter must be 'int', 'string', etc."
    return vtk.vtkVariant(v, i)

def vtkVariantExtract(v, t=None):
    """
    Extract the specified value type from the vtkVariant, where the type is
    int the following format:    'int', 'unsigned int', etc. for numerics,
    'string', and 'unicode string'.
    """
    if t == None:
        if v.IsVTKObject():
             raise ValueError, "use ToVTKObject() instead"
        t = v.GetTypeAsString()
    try:
        i = _variant_type_map[t]
    except KeyError:
        raise ValueError, "second parameter must be 'int', 'string', etc."
    if getattr(v, _variant_check_map[t])():
        return getattr(v, _variant_method_map[t])()
    else:
        return None

def vtkVariantCast(v, t):
    """
    Cast the vtkVariant to the specified value type, where the type is
    int the following format:    'int', 'unsigned int', etc. for numerics,
    'string', and 'unicode string'.
    """
    try:
        i = _variant_type_map[t]
    except KeyError:
        raise ValueError, "second parameter must be 'int', 'string', etc."
    u = vtk.vtkVariant(v, i)
    if u.IsValid():
        return getattr(u, _variant_method_map[t])()
    else:
        return None
