"""
Miscellaneous constants copied from vtkSetGet.h
(It would be nice if this file were automatically generated,
but the constants don't change very often)
"""

# Some constants used throughout code

VTK_LARGE_FLOAT = 1.0e+38
VTK_LARGE_INTEGER = 2147483647 # 2^31 - 1

# These types are returned by GetDataType to indicate pixel type.
VTK_VOID            = 0
VTK_BIT             = 1
VTK_CHAR            = 2
VTK_UNSIGNED_CHAR   = 3
VTK_SHORT           = 4
VTK_UNSIGNED_SHORT  = 5
VTK_INT             = 6
VTK_UNSIGNED_INT    = 7
VTK_LONG            = 8
VTK_UNSIGNED_LONG   = 9
VTK_FLOAT           =10
VTK_DOUBLE          =11

# Some constant required for correct template performance
VTK_BIT_MIN = 0
VTK_BIT_MAX = 1
VTK_CHAR_MIN = -128
VTK_CHAR_MAX = 127
VTK_UNSIGNED_CHAR_MIN = 0
VTK_UNSIGNED_CHAR_MAX = 255
VTK_SHORT_MIN = -32768
VTK_SHORT_MAX = 32767
VTK_UNSIGNED_SHORT_MIN = 0
VTK_UNSIGNED_SHORT_MAX = 65535
VTK_INT_MIN = (-VTK_LARGE_INTEGER-1)
VTK_INT_MAX = VTK_LARGE_INTEGER
#VTK_UNSIGNED_INT_MIN = 0
#VTK_UNSIGNED_INT_MAX = 4294967295
VTK_LONG_MIN = (-VTK_LARGE_INTEGER-1)
VTK_LONG_MAX = VTK_LARGE_INTEGER
#VTK_UNSIGNED_LONG_MIN = 0
#VTK_UNSIGNED_LONG_MAX = 4294967295
VTK_FLOAT_MIN = -VTK_LARGE_FLOAT
VTK_FLOAT_MAX = VTK_LARGE_FLOAT
VTK_DOUBLE_MIN = -1.0e+99
VTK_DOUBLE_MAX  = 1.0e+99

# These types are returned to distinguish dataset types
VTK_POLY_DATA          = 0
VTK_STRUCTURED_POINTS  = 1
VTK_STRUCTURED_GRID    = 2
VTK_RECTILINEAR_GRID   = 3
VTK_UNSTRUCTURED_GRID  = 4

# A macro to get the name of a type

__vtkTypeNameDict = {VTK_VOID:"void",
                     VTK_DOUBLE:"double",
                     VTK_FLOAT:"float",
                     VTK_LONG:"long",
                     VTK_UNSIGNED_LONG:"unsigned long",
                     VTK_INT:"int",
                     VTK_UNSIGNED_INT:"unsigned int",
                     VTK_SHORT:"short",
                     VTK_UNSIGNED_SHORT:"unsigned short",
                     VTK_CHAR:"char",
                     VTK_UNSIGNED_CHAR:"unsigned char",
                     VTK_BIT:"bit"}
                   
def vtkImageScalarTypeNameMacro(type):
  return __vtkTypeNameDict[type]


