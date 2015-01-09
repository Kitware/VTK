/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cstdlib>

#include "ADIOSAttribute.h"

namespace ADIOS
{

//----------------------------------------------------------------------------
Attribute::Attribute(ADIOS_FILE *f, int id)
: Id(id), Name(f->attr_namelist[id])
{
  int err, typeSize;

  err = adios_get_attr_byid(f, id, &this->Type, &typeSize, &this->Value);
  ReadError::TestEq(0, err);
}

//----------------------------------------------------------------------------
Attribute::~Attribute()
{
  if(this->Value)
    {
    std::free(this->Value);
    }
}

//----------------------------------------------------------------------------
const int& Attribute::GetId() const
{
  return this->Id;
}

//----------------------------------------------------------------------------
const ADIOS_DATATYPES& Attribute::GetType() const
{
  return this->Type;
}

//----------------------------------------------------------------------------
const std::string& Attribute::GetName(void) const
{
  return this->Name;
}

//----------------------------------------------------------------------------
template<>
const std::string Attribute::GetValue<std::string>() const
{
  ReadError::TestEq(this->Type, adios_string, "Invalid type");
  return reinterpret_cast<const char*>(this->Value);
}

} // End namespace ADIOS
