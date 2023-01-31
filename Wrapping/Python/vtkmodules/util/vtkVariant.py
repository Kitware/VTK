"""
Utility functions to mimic the template support functions for vtkVariant
"""

from vtkmodules import vtkCommonCore
import sys

_variant_type_map = {
    'void' : vtkCommonCore.VTK_VOID,
    'char' : vtkCommonCore.VTK_CHAR,
    'unsigned char' : vtkCommonCore.VTK_UNSIGNED_CHAR,
    'signed char' : vtkCommonCore.VTK_SIGNED_CHAR,
    'short' : vtkCommonCore.VTK_SHORT,
    'unsigned short' : vtkCommonCore.VTK_UNSIGNED_SHORT,
    'int' : vtkCommonCore.VTK_INT,
    'unsigned int' : vtkCommonCore.VTK_UNSIGNED_INT,
    'long' : vtkCommonCore.VTK_LONG,
    'unsigned long' : vtkCommonCore.VTK_UNSIGNED_LONG,
    'long long' : vtkCommonCore.VTK_LONG_LONG,
    'unsigned long long' : vtkCommonCore.VTK_UNSIGNED_LONG_LONG,
    'float' : vtkCommonCore.VTK_FLOAT,
    'double' : vtkCommonCore.VTK_DOUBLE,
    'string' : vtkCommonCore.VTK_STRING,
    'vtkObjectBase' : vtkCommonCore.VTK_OBJECT,
    'vtkObject' : vtkCommonCore.VTK_OBJECT,
}

_variant_method_map = {
    vtkCommonCore.VTK_VOID : '',
    vtkCommonCore.VTK_CHAR : 'ToChar',
    vtkCommonCore.VTK_UNSIGNED_CHAR : 'ToUnsignedChar',
    vtkCommonCore.VTK_SIGNED_CHAR : 'ToSignedChar',
    vtkCommonCore.VTK_SHORT : 'ToShort',
    vtkCommonCore.VTK_UNSIGNED_SHORT : 'ToUnsignedShort',
    vtkCommonCore.VTK_INT : 'ToInt',
    vtkCommonCore.VTK_UNSIGNED_INT : 'ToUnsignedInt',
    vtkCommonCore.VTK_LONG : 'ToLong',
    vtkCommonCore.VTK_UNSIGNED_LONG : 'ToUnsignedLong',
    vtkCommonCore.VTK_LONG_LONG : 'ToLongLong',
    vtkCommonCore.VTK_UNSIGNED_LONG_LONG : 'ToUnsignedLongLong',
    vtkCommonCore.VTK_FLOAT : 'ToFloat',
    vtkCommonCore.VTK_DOUBLE : 'ToDouble',
    vtkCommonCore.VTK_STRING : 'ToString',
    vtkCommonCore.VTK_OBJECT : 'ToVTKObject',
}

_variant_check_map = {
    vtkCommonCore.VTK_VOID : 'IsValid',
    vtkCommonCore.VTK_CHAR : 'IsChar',
    vtkCommonCore.VTK_UNSIGNED_CHAR : 'IsUnsignedChar',
    vtkCommonCore.VTK_SIGNED_CHAR : 'IsSignedChar',
    vtkCommonCore.VTK_SHORT : 'IsShort',
    vtkCommonCore.VTK_UNSIGNED_SHORT : 'IsUnsignedShort',
    vtkCommonCore.VTK_INT : 'IsInt',
    vtkCommonCore.VTK_UNSIGNED_INT : 'IsUnsignedInt',
    vtkCommonCore.VTK_LONG : 'IsLong',
    vtkCommonCore.VTK_UNSIGNED_LONG : 'IsUnsignedLong',
    vtkCommonCore.VTK_LONG_LONG : 'IsLongLong',
    vtkCommonCore.VTK_UNSIGNED_LONG_LONG : 'IsUnsignedLongLong',
    vtkCommonCore.VTK_FLOAT : 'IsFloat',
    vtkCommonCore.VTK_DOUBLE : 'IsDouble',
    vtkCommonCore.VTK_STRING : 'IsString',
    vtkCommonCore.VTK_OBJECT : 'IsVTKObject',
}


def vtkVariantCreate(v, t):
    """
    Create a vtkVariant of the specified type, where the type is in the
    following format: 'int', 'unsigned int', etc. for numeric types,
    and 'string' for strings.  You can also use an
    integer VTK type constant for the type.
    """
    if not issubclass(type(t), int):
        t = _variant_type_map[t]

    return vtkCommonCore.vtkVariant(v, t)


def vtkVariantExtract(v, t=None):
    """
    Extract the specified value type from the vtkVariant, where the type is
    in the following format: 'int', 'unsigned int', etc. for numeric types,
    and 'string' for strings.  You can also use an
    integer VTK type constant for the type.  Set the type to 'None" to
    extract the value in its native type.
    """
    v = vtkCommonCore.vtkVariant(v)

    if t == None:
        t = v.GetType()
    elif not issubclass(type(t), int):
        t = _variant_type_map[t]

    if getattr(v, _variant_check_map[t])():
        return getattr(v, _variant_method_map[t])()
    else:
        return None


def vtkVariantCast(v, t):
    """
    Cast the vtkVariant to the specified value type, where the type is
    in the following format: 'int', 'unsigned int', etc. for numeric types,
    and 'string' for strings.  You can also use an
    integer VTK type constant for the type.
    """
    if not issubclass(type(t), int):
        t = _variant_type_map[t]

    v = vtkCommonCore.vtkVariant(v, t)

    if v.IsValid():
        return getattr(v, _variant_method_map[t])()
    else:
        return None


def vtkVariantStrictWeakOrder(s1, s2):
    """
    Compare variants by type first, and then by value.
    """
    s1 = vtkCommonCore.vtkVariant(s1)
    s2 = vtkCommonCore.vtkVariant(s2)

    t1 = s1.GetType()
    t2 = s2.GetType()

    # check based on type
    if t1 != t2:
        return t1 < t2

    v1 = s1.IsValid()
    v2 = s2.IsValid()

    # check based on validity
    if (not v1) or (not v2):
        return v1 < v2

    # extract and compare the values
    r1 = getattr(s1, _variant_method_map[t1])()
    r2 = getattr(s2, _variant_method_map[t2])()

    # compare vtk objects by classname, then address
    if t1 == vtkCommonCore.VTK_OBJECT:
        c1 = r1.GetClassName()
        c2 = r2.GetClassName()
        if c1 != c2:
            return c1 < c2
        else:
            return r1.__this__ < r2.__this__

    return r1 < r2


class vtkVariantStrictWeakOrderKey:
    """A key method (class, actually) for use with sort()"""
    def __init__(self, obj, *args):
        self.obj = obj
    def __lt__(self, other):
        return vtkVariantStrictWeakOrder(self.obj, other)


def vtkVariantStrictEquality(s1, s2):
    """
    Check two variants for strict equality of type and value.
    """
    s1 = vtkCommonCore.vtkVariant(s1)
    s2 = vtkCommonCore.vtkVariant(s2)

    t1 = s1.GetType()
    t2 = s2.GetType()

    # check based on type
    if t1 != t2:
        return False

    v1 = s1.IsValid()
    v2 = s2.IsValid()

    # check based on validity
    if (not v1) and (not v2):
        return True
    elif v1 != v2:
        return False

    # extract and compare the values
    r1 = getattr(s1, _variant_method_map[t1])()
    r2 = getattr(s2, _variant_method_map[t2])()

    return (r1 == r2)


def vtkVariantLessThan(s1, s2):
    """
    Return true if s1 < s2.
    """
    return (vtkCommonCore.vtkVariant(s1) < vtkCommonCore.vtkVariant(s2))


def vtkVariantEqual(s1, s2):
    """
    Return true if s1 == s2.
    """
    return (vtkCommonCore.vtkVariant(s1) == vtkCommonCore.vtkVariant(s2))
