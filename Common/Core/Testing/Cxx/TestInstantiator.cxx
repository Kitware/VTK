/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestInstantiator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkCommonInstantiator
// .SECTION Description
// Make sure common registers its classes with vtkInstantiator.

#include "vtkCommonInstantiator.h"

int main()
{
  int result = 0;
  vtkObject* object = vtkInstantiator::CreateInstance("vtkDoubleArray");
  if(object)
    {
    if(object->IsA("vtkDoubleArray"))
      {
      cout << "Successfully created an instance of vtkDoubleArray." << endl;
      }
    else
      {
      cerr << "Created an instance of " << object->GetClassName()
           << "instead of vtkDoubleArray." << endl;
      result = 1;
      }
    object->Delete();
    }
  else
    {
    cerr << "Failed to create an instance of vtkDoubleArray." << endl;
    result = 1;
    }
  return result;
}
