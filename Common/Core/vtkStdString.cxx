/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStdString.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStdString.h"

//----------------------------------------------------------------------------
ostream& operator<<(ostream& os, const vtkStdString& s)
{
  return os << s.c_str();
}
