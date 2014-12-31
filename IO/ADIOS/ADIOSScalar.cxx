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

//----------------------------------------------------------------------------
Scalar::Scalar(ADIOS_FILE *f, ADIOS_VARINFO *v)
: VarInfo(f, v), Values(NULL)
{
  // Allocate memory
  switch(this->Type)
    {
    case adios_byte:
      this->Values = new int8_t[v->sum_nblocks];
      break;
    case adios_short:
      this->Values = new int16_t[v->sum_nblocks];
      break;
    case adios_integer:
      this->Values = new int32_t[v->sum_nblocks];
      break;
    case adios_long:
      this->Values = new int64_t[v->sum_nblocks];
      break;
    case adios_unsigned_byte:
      this->Values = new uint8_t[v->sum_nblocks];
      break;
    case adios_unsigned_short:
      this->Values = new uint16_t[v->sum_nblocks];
      break;
    case adios_unsigned_integer:
      this->Values = new uint32_t[v->sum_nblocks];
      break;
    case adios_unsigned_long:
      this->Values = new uint64_t[v->sum_nblocks];
      break;
    case adios_real:
      this->Values = new float[v->sum_nblocks];
      break;
    case adios_double:
      this->Values = new double[v->sum_nblocks];
      break;
    case adios_complex:
      this->Values = new std::complex<float>[v->sum_nblocks];
      break;
    case adios_double_complex:
      this->Values = new std::complex<double>[v->sum_nblocks];
      break;
    default: break;
    }
  size_t tSize = Type::SizeOf(this->Type);

  // Read all blocks and steps
  int err;
  char *rawPtr = reinterpret_cast<char *>(this->Values);
  for(size_t s = 0; s < v->nsteps; ++s)
    {
    for(size_t b = 0; b < v->nblocks[s]; ++b)
      {
      ADIOS_SELECTION *sel = adios_selection_writeblock(b);
      ReadError::TestNe<ADIOS_SELECTION*>(NULL, sel);

      err = adios_schedule_read_byid(f, sel, v->varid, s, 1, rawPtr);
      ReadError::TestEq(0, err);

      err = adios_perform_reads(f, 1);
      ReadError::TestEq(0, err);

      adios_selection_delete(sel);
      rawPtr += tSize;
      }
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
