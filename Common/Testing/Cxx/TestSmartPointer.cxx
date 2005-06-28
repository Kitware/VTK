/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartPointer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSmartPointer.
// .SECTION Description
// Tests instantiations of the vtkSmartPointer class template.

#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"

int TestSmartPointer(int,char *[])
{
#if !defined(VTK_LEGACY_REMOVE) && defined(VTK_LEGACY_SILENT)
  vtkDebugLeaks::PromptUserOff();
#endif
  
  vtkIntArray* ia = vtkIntArray::New();
  
  // Coverage:
  vtkSmartPointer<vtkDataArray> da1;
  vtkSmartPointer<vtkIntArray>  da2(ia);
  vtkSmartPointer<vtkFloatArray> da3(da1);
  da1 = ia;
  da2 = da1;
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
  da1 = vtkSmartPointer<vtkDataArray>::NewInstance(ia);
  
  ia->Delete();
  
  return 0;
} 
