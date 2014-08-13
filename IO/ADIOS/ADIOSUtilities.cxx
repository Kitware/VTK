#include "ADIOSUtilities.h"
#include <stdint.h>
#include <limits>
#include <complex>
#include <string>

#define INSTANTIATE(TN, TA) \
template<> ADIOS_DATATYPES ADIOSUtilities::TypeNativeToADIOS<TN>::T = TA;
INSTANTIATE(int8_t, adios_byte)
INSTANTIATE(int16_t, adios_short)
INSTANTIATE(int32_t, adios_integer)
//INSTANTIATE(int64_t, adios_long)
INSTANTIATE(uint8_t, adios_unsigned_byte)
INSTANTIATE(uint16_t, adios_unsigned_short)
INSTANTIATE(uint32_t, adios_unsigned_integer)
INSTANTIATE(uint64_t, adios_unsigned_long)
INSTANTIATE(vtkIdType, adios_long)
INSTANTIATE(float, adios_real)
INSTANTIATE(double, adios_double)
INSTANTIATE(std::complex<float>, adios_complex)
INSTANTIATE(std::complex<double>, adios_double_complex)
INSTANTIATE(std::string, adios_string)
#undef INSTANTIATE

#define INSTANTIATE(TN, TV) \
template<> int ADIOSUtilities::TypeNativeToVTK<TN>::T = TV;
INSTANTIATE(int8_t, VTK_TYPE_INT8)
INSTANTIATE(int16_t, VTK_TYPE_INT16)
INSTANTIATE(int32_t, VTK_TYPE_INT32)
//INSTANTIATE(int64_t, VTK_TYPE_INT64)
INSTANTIATE(uint8_t, VTK_TYPE_UINT8)
INSTANTIATE(uint16_t, VTK_TYPE_UINT16)
INSTANTIATE(uint32_t, VTK_TYPE_UINT32)
INSTANTIATE(uint64_t, VTK_TYPE_UINT64)
INSTANTIATE(vtkIdType, VTK_ID_TYPE)
INSTANTIATE(float, VTK_FLOAT)
INSTANTIATE(double, VTK_DOUBLE)
INSTANTIATE(std::string, VTK_STRING)
#undef INSTANTIATE

ADIOS_DATATYPES ADIOSUtilities::TypeVTKToADIOS(int tv)
{
  switch(tv)
    {
    case VTK_TYPE_INT8: return adios_byte;
    case VTK_TYPE_INT16: return adios_short;
    case VTK_TYPE_INT32: return adios_integer;
    case VTK_TYPE_INT64: return adios_long;
    case VTK_TYPE_UINT8: return adios_unsigned_byte;
    case VTK_TYPE_UINT16: return adios_unsigned_short;
    case VTK_TYPE_UINT32: return adios_unsigned_integer;
    case VTK_TYPE_UINT64: return adios_unsigned_long;
    case VTK_FLOAT: return adios_real;
    case VTK_DOUBLE: return adios_double;
    case VTK_STRING: return adios_string;
    case VTK_ID_TYPE:
      switch(sizeof(vtkIdType))
        {
        case 1: return adios_byte;
        case 2: return adios_short;
        case 4: return adios_integer;
        case 8: return adios_long;
        default: return adios_unknown;
        }
    default: return adios_unknown;
    }
}

int ADIOSUtilities::TypeADIOSToVTK(ADIOS_DATATYPES ta)
{
  switch(ta)
    {
    case adios_byte: return VTK_TYPE_INT8;
    case adios_short: return VTK_TYPE_INT16;
    case adios_integer: return VTK_TYPE_INT32;
    case adios_long: return VTK_ID_TYPE;
    case adios_unsigned_byte: return VTK_TYPE_UINT8;
    case adios_unsigned_short: return VTK_TYPE_UINT16;
    case adios_unsigned_integer: return VTK_TYPE_UINT32;
    case adios_unsigned_long: return VTK_TYPE_UINT64;
    case adios_real: return VTK_FLOAT;
    case adios_double: return VTK_DOUBLE;
    case adios_string: return VTK_STRING;
    default:  return -1;
    }
}

size_t ADIOSUtilities::TypeSize(ADIOS_DATATYPES ta)
{
  switch(ta)
    {
    case adios_byte: return 1;
    case adios_short: return 2;
    case adios_integer: return 4;
    case adios_long: return 8;
    case adios_unsigned_byte: return 1;
    case adios_unsigned_short: return 2;
    case adios_unsigned_integer: return 4;
    case adios_unsigned_long: return 8;
    case adios_real: return 4;
    case adios_double: return 8;
    case adios_complex: return 8;
    case adios_double_complex: return 16;
    case adios_string: return 1;
    default: return 0;
    }
}
