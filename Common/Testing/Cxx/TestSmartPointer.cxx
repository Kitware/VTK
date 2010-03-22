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
#include "vtkNew.h"
#include "vtkSmartPointer.h"

template <class T>
bool CheckRefCount(const char* v, int line, T const& o, int n)
{
  int c = o->GetReferenceCount();
  if(c != n)
    {
    cerr << "At line " << line << ": "
         << "RefCount of " << v << " is " << c << ", not " << n << endl;
    return false;
    }
  return true;
}

#define CheckRefCount(obj, n) CheckRefCount(#obj, __LINE__, obj, n)

bool TestNew()
{
  {
  vtkSmartPointer<vtkDataArray> da1 = vtkNew<vtkFloatArray>();
  if(!CheckRefCount(da1, 1)) { return false; }
  }
  vtkSmartPointer<vtkDataArray> da2;
  vtkDataArray* da3 = 0;
  {
  vtkNew<vtkIntArray> ia1;
  cout << "IntArray: " << ia1 << "\n";
  if(!CheckRefCount(ia1, 1)) { return false; }
  da2 = ia1;
  da3 = ia1;
  if(!CheckRefCount(ia1, 2)) { return false; }
  }
  if(!CheckRefCount(da3, 1)) { return false; }
  return da2 == da3;
}

int TestSmartPointer(int,char *[])
{
  vtkIntArray* ia = vtkIntArray::New();
  
  // Coverage:
  vtkSmartPointer<vtkIntArray>  da2(ia);
  vtkSmartPointer<vtkFloatArray> da3;
  vtkSmartPointer<vtkDataArray> da1(da2);
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
  da1 = vtkSmartPointer<vtkDataArray>::NewInstance(ia);
  da1.TakeReference(vtkIntArray::New());
  vtkSmartPointer<vtkIntArray> da4 =
    vtkSmartPointer<vtkIntArray>::Take(vtkIntArray::New());
  (void)da4;
  ia->Delete();

  if(!TestNew())
    {
    return 1;
    }

  return 0;
} 
