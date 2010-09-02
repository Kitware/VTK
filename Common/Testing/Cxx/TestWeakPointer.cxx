/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestWeakPointer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkWeakPointer.
// .SECTION Description
// Tests instantiations of the vtkWeakPointer class template.

#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkWeakPointer.h"

int TestWeakPointer(int,char *[])
{
  vtkIntArray* ia = vtkIntArray::New();

  // Coverage:
  vtkWeakPointer<vtkIntArray>  da2(ia);
  vtkWeakPointer<vtkFloatArray> da3;
  vtkWeakPointer<vtkDataArray> da1(da2);
  da1 = ia;
  da1 = da2;
  da2 == da3;
  da2 != da3;
  da2 < da3;
  da2 <= da3;
  da2 > da3;
  da2 >= da3;
  ia == da3;
  ia != da3;
  ia < da3;
  ia <= da3;
  ia > da3;
  ia >= da3;
  da2 == ia;
  da2 != ia;
  da2 < ia;
  da2 <= ia;
  da2 > ia;
  da2 >= ia;
  da1 == 0;
  da1 != 0;
  da1 < 0;
  da1 <= 0;
  da1 > 0;
  da1 >= 0;
  (*da1).SetNumberOfComponents(1);
  if(da2)
    {
    da2->SetNumberOfComponents(1);
    }
  if(!da2)
    {
    cerr << "da2 is NULL!" << "\n";
    return 1;
    }
  cout << "IntArray: " << da2 << "\n";

  if (da1.GetPointer() == 0)
    {
    cerr << "da1.GetPointer() is NULL\n";
    }
  if (da2.GetPointer() == 0)
    {
    cerr << "da2.GetPointer() is NULL\n";
    }
  if (da3.GetPointer() != 0)
    {
    cerr << "da3.GetPointer() is not NULL\n";
    }

  da2 = 0;
  ia->Delete();

  if (da1.GetPointer() != NULL)
    {
    cerr << "da1.GetPointer() is not NULL\n";
    }

  return 0;
}
