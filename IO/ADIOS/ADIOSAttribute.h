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
// .NAME ADIOSAttribute - The utility class wrapping static ADIOS sttributes

#ifndef _ADIOSAttribute_h
#define _ADIOSAttribute_h

#include <string>
#include <vector>
#include <adios_types.h>

struct ADIOSAttributeImpl;

//----------------------------------------------------------------------------
class ADIOSAttribute
{
public:
  ADIOSAttribute(ADIOSAttributeImpl *impl);
  ~ADIOSAttribute(void);

  const std::string& GetName(void) const;
  int GetId(void) const;
  ADIOS_DATATYPES GetType(void) const;

  template<typename T>
  T GetValue(void) const;

private:
  ADIOSAttributeImpl *Impl;
};

#endif // _ADIOSAttribute_h
// VTK-HeaderTest-Exclude: ADIOSAttribute.h
