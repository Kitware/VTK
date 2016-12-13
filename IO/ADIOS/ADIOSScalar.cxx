/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSScalar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <complex>
#include <stdexcept>

#include <adios_read.h>

#include "ADIOSScalar.h"

namespace ADIOS
{

template<typename T>
void LoadScalarsFromStats(void* &ptr, ADIOS_VARINFO *v)
{
  T* &ptrT = reinterpret_cast<T*&>(ptr);
  ptrT = new T[v->sum_nblocks];
  for(int i = 0; i < v->sum_nblocks; ++i)
  {
    ptrT[i] = *reinterpret_cast<const T*>(v->statistics->blocks->mins[i]);
  }
}

//----------------------------------------------------------------------------
Scalar::Scalar(ADIOS_FILE *f, ADIOS_VARINFO *v)
: VarInfo(f, v), Values(NULL)
{
  switch(this->Type)
  {
    case adios_byte:
      LoadScalarsFromStats<int8_t>(this->Values, v);
      break;
    case adios_short:
      LoadScalarsFromStats<int16_t>(this->Values, v);
      break;
    case adios_integer:
      LoadScalarsFromStats<int32_t>(this->Values, v);
      break;
    case adios_long:
      LoadScalarsFromStats<int64_t>(this->Values, v);
      break;
    case adios_unsigned_byte:
      LoadScalarsFromStats<uint8_t>(this->Values, v);
      break;
    case adios_unsigned_short:
      LoadScalarsFromStats<uint16_t>(this->Values, v);
      break;
    case adios_unsigned_integer:
      LoadScalarsFromStats<uint32_t>(this->Values, v);
      break;
    case adios_unsigned_long:
      LoadScalarsFromStats<uint64_t>(this->Values, v);
      break;
    case adios_real:
      LoadScalarsFromStats<float>(this->Values, v);
      break;
    case adios_double:
      LoadScalarsFromStats<double>(this->Values, v);
      break;
    case adios_complex:
      LoadScalarsFromStats<std::complex<float> >(this->Values, v);
      break;
    case adios_double_complex:
      LoadScalarsFromStats<std::complex<double> >(this->Values, v);
      break;
    default:
      // Unsupported data type
      break;
  }
}

//----------------------------------------------------------------------------
Scalar::~Scalar()
{
  if(!this->Values)
  {
    return;
  }

  switch(this->Type)
  {
    case adios_byte:
      delete[] reinterpret_cast<int8_t*>(this->Values);
      break;
    case adios_short:
      delete[] reinterpret_cast<int16_t*>(this->Values);
      break;
    case adios_integer:
      delete[] reinterpret_cast<int32_t*>(this->Values);
      break;
    case adios_long:
      delete[] reinterpret_cast<int64_t*>(this->Values);
      break;
    case adios_unsigned_byte:
      delete[] reinterpret_cast<uint8_t*>(this->Values);
      break;
    case adios_unsigned_short:
      delete[] reinterpret_cast<uint16_t*>(this->Values);
      break;
    case adios_unsigned_integer:
      delete[] reinterpret_cast<uint32_t*>(this->Values);
      break;
    case adios_unsigned_long:
      delete[] reinterpret_cast<uint64_t*>(this->Values);
      break;
    case adios_real:
      delete[] reinterpret_cast<float*>(this->Values);
      break;
    case adios_double:
      delete[] reinterpret_cast<double*>(this->Values);
      break;
    case adios_complex:
      delete[] reinterpret_cast<std::complex<float>*>(this->Values);
      break;
    case adios_double_complex:
      delete[] reinterpret_cast<std::complex<double>*>(this->Values);
      break;
    default: break;
  }
}

} // End namespace ADIOS
