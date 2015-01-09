/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSAttribute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef _ADIOSAttribute_h
#define _ADIOSAttribute_h

#include <string>
#include <vector>

#include <adios_read.h>

#include "ADIOSUtilities.h"

namespace ADIOS
{

class Attribute
{
public:
  Attribute(ADIOS_FILE *f, int id);
  ~Attribute(void);

  const int& GetId() const;
  const ADIOS_DATATYPES& GetType() const;
  const std::string& GetName(void) const;

  template<typename T>
  const T GetValue() const
  {
    ReadError::TestEq(this->Type, Type::NativeToADIOS<T>(), "Invalid type");

    return *reinterpret_cast<const T*>(this->Value);
  }

protected:
  int Id;
  ADIOS_DATATYPES Type;
  std::string Name;
  void* Value;
};
template<> const std::string Attribute::GetValue<std::string>() const;

} // End namespace ADIOS
#endif // _ADIOSAttribute_h
// VTK-HeaderTest-Exclude: ADIOSAttribute.h
