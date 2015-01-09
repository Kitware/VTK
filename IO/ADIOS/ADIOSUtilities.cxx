/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkType.h>

#include <adios.h>
#include <adios_read.h>

#include "ADIOSUtilities.h"

namespace ADIOS
{

//----------------------------------------------------------------------------
WriteError::WriteError(const std::string& msg)
: std::runtime_error(msg.empty() ? adios_get_last_errmsg() : msg)
{
  if(msg.empty())
    {
    adios_clear_error();
    }
}

//----------------------------------------------------------------------------
ReadError::ReadError(const std::string& msg)
: std::runtime_error(msg.empty() ? adios_errmsg() : msg)
{
}


namespace Type
{

//----------------------------------------------------------------------------
template<> ADIOS_DATATYPES SizeToInt<1>() { return adios_byte; }
template<> ADIOS_DATATYPES SizeToInt<2>() { return adios_short; }
template<> ADIOS_DATATYPES SizeToInt<4>() { return adios_integer; }
template<> ADIOS_DATATYPES SizeToInt<8>() { return adios_long; }
template<> ADIOS_DATATYPES SizeToUInt<1>() { return adios_unsigned_byte; }
template<> ADIOS_DATATYPES SizeToUInt<2>() { return adios_unsigned_short; }
template<> ADIOS_DATATYPES SizeToUInt<4>() { return adios_unsigned_integer; }
template<> ADIOS_DATATYPES SizeToUInt<8>() { return adios_unsigned_long; }

//----------------------------------------------------------------------------
#define INSTANTIATE(TN, TA) \
template<> ADIOS_DATATYPES NativeToADIOS<TN>() { return TA; }
INSTANTIATE(int8_t, adios_byte)
INSTANTIATE(int16_t, adios_short)
INSTANTIATE(int32_t, adios_integer)
INSTANTIATE(int64_t, adios_long)
INSTANTIATE(uint8_t, adios_unsigned_byte)
INSTANTIATE(uint16_t, adios_unsigned_short)
INSTANTIATE(uint32_t, adios_unsigned_integer)
INSTANTIATE(uint64_t, adios_unsigned_long)
INSTANTIATE(float, adios_real)
INSTANTIATE(double, adios_double)
INSTANTIATE(std::complex<float>, adios_complex)
INSTANTIATE(std::complex<double>, adios_double_complex)
INSTANTIATE(std::string, adios_string)
#undef INSTANTIATE

//----------------------------------------------------------------------------
size_t SizeOf(ADIOS_DATATYPES ta)
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

//----------------------------------------------------------------------------
bool IsInt(ADIOS_DATATYPES ta)
{
  return ta == adios_byte             || ta == adios_short ||
         ta == adios_integer          || ta == adios_long ||
         ta == adios_unsigned_byte    || ta == adios_unsigned_short ||
         ta == adios_unsigned_integer || ta == adios_unsigned_long;
}

} // End namespace Tye
} // End namespace ADIOS
