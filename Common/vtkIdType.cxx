/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdType.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSystemIncludes.h" // Cannot include vtkIdType.h directly.

#include "vtkIOStream.h"

// Visual Studio 6 does not provide these operators.
#if defined(VTK_USE_64BIT_IDS) && defined(_MSC_VER) && (_MSC_VER < 1300)

//----------------------------------------------------------------------------
ostream& vtkIdTypeOutput(ostream& os, __int64 id)
{
  // _i64toa can use up to 33 bytes (32 + null terminator).
  char buf[33];
  // Convert to string representation in base 10.
  return (os << _i64toa(id, buf, 10));
}

//----------------------------------------------------------------------------
istream& vtkIdTypeInput(istream& is, __int64& id)
{
  // Up to 33 bytes may be needed (32 + null terminator).
  char buf[33];
  is.width(33);
  
  // Read the string representation from the input.
  if(is >> buf)
    {
    // Convert from string representation to integer.
    id = _atoi64(buf);
    }
  return is;
}

#endif
