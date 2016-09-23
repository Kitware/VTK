/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkADIOSUtilities.h"

namespace ADIOS
{
namespace Type
{

//----------------------------------------------------------------------------
template<> ADIOS_DATATYPES NativeToADIOS<vtkIdType>()
{
  return SizeToInt<sizeof(vtkIdType)>();
}

//----------------------------------------------------------------------------
ADIOS_DATATYPES VTKToADIOS(int tv)
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

//----------------------------------------------------------------------------
int ADIOSToVTK(ADIOS_DATATYPES ta)
{
  switch(ta)
  {
    case adios_byte: return VTK_TYPE_INT8;
    case adios_short: return VTK_TYPE_INT16;
    case adios_integer: return VTK_TYPE_INT32;
    case adios_long: return VTK_TYPE_INT64;
    case adios_unsigned_byte: return VTK_TYPE_UINT8;
    case adios_unsigned_short: return VTK_TYPE_UINT16;
    case adios_unsigned_integer: return VTK_TYPE_UINT32;
    case adios_unsigned_long: return VTK_TYPE_UINT64;
    case adios_real: return VTK_FLOAT;
    case adios_double: return VTK_DOUBLE;
    case adios_string: return VTK_STRING;
    default: return -1;
  }
}

} // End namespace Type
} // End namespace ADIOS
