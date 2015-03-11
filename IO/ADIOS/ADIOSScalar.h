/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSScalar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME ADIOSScalar - The utility class wrapping the ADIOS_VARINFO struct

#ifndef _ADIOSScalar_h
#define _ADIOSScalar_h

#include <string>
#include <vector>

#include "ADIOSUtilities.h"

#include "ADIOSVarInfo.h"

//----------------------------------------------------------------------------
namespace ADIOS
{

class Scalar : public VarInfo
{
public:
  Scalar(ADIOS_FILE *f, ADIOS_VARINFO *v);
  virtual ~Scalar(void);

  template<typename T>
  const T& GetValue(size_t step, size_t block) const
  {
    ReadError::TestEq(this->Type, Type::NativeToADIOS<T>(), "Invalid type");

    StepBlock* idx = this->GetNewestBlockIndex(step, block);
    ReadError::TestNe<StepBlock*>(NULL, idx, "Variable not available");

    return reinterpret_cast<const T*>(this->Values)[idx->BlockId];
  }

protected:
  void *Values;
};

} // End namespace ADIOS
#endif // _ADIOSScalar_h
// VTK-HeaderTest-Exclude: ADIOSScalar.h
