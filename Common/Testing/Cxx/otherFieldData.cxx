/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherFieldData.cxx
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

#include "vtkDebugLeaks.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"

int main()
{
  vtkDebugLeaks::PromptUserOff();

  vtkFieldData* fd = vtkFieldData::New();

  vtkFloatArray* fa;

  char name[128];
  for(int i=0; i<5; i++)
    {
    sprintf(name, "Array%d", i);
    fa = vtkFloatArray::New();
    fa->SetName(name);
    fd->AddArray(fa);
    fa->Delete();
    }

  // Coverage
  vtkFieldData::Iterator it(fd);
  vtkFieldData::Iterator it2(it);

  it = it;
  it2 = it;

  fd->Allocate(20);
  fd->CopyFieldOff("Array0");
  fd->CopyFieldOff("Array1");

  vtkFieldData* fd2 = fd->MakeObject();
  fd2->ShallowCopy(fd);
  fd2->DeepCopy(fd);

  vtkIdList* ptIds = vtkIdList::New();
  ptIds->InsertNextId(0);
  ptIds->InsertNextId(2);

  fd->GetField(ptIds, fd2);
  ptIds->Delete();

  int arrayComp;
  cout << fd->GetArrayContainingComponent(1, arrayComp) << endl;

  float tuple[10];
  fd->GetTuple(2);
  fd->SetTuple(2, tuple);
  fd->InsertTuple(2, tuple);
  fd->InsertNextTuple(tuple);
  fd->SetComponent(0,0, 1.0);
  fd->InsertComponent(0,0, 1.0);

  fd2->Reset();

  fd->Delete();
  fd2->Delete();

  return 0;

} 
