/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestConvertTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

typedef VTK_TEST_CONVERT_TYPE_TO TypeTo;
typedef VTK_TEST_CONVERT_TYPE_FROM TypeFrom;

void function(TypeTo& l, TypeFrom const& r)
{
  l = static_cast<TypeTo>(r);
}

int main()
{
  TypeTo tTo = TypeTo();
  TypeFrom tFrom = TypeFrom();
  function(tTo, tFrom);
  return 0;
}
