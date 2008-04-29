/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFastNumericConversion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkFastNumericConversion.
// .SECTION Description
// Tests performance of the vtkFastNumericConversion methods.

#include "vtkFastNumericConversion.h"

int TestFastNumericConversion(int,char *[])
{
  vtkFastNumericConversion* fnc = vtkFastNumericConversion::New();
  fnc->PerformanceTests();
  fnc->Print(cout);
  fnc->Delete();
  return 0;
}
