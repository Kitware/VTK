/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

int otherFieldData(int,char *[])
{
  int i;
  vtkFieldData* fd = vtkFieldData::New();

  vtkFloatArray* fa;

  char name[128];
  for(i=0; i<5; i++)
  {
    snprintf(name, sizeof(name), "Array%d", i);
    fa = vtkFloatArray::New();
    fa->SetName(name);
    // the tuples must be set before being read to avoid a UMR
    // this must have been a UMR in the past that was suppressed
    fa->Allocate(20);
    fa->SetTuple1(0,0.0);
    fa->SetTuple1(2,0.0);
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

  vtkFieldData* fd2 = fd->NewInstance();
  fd2->CopyStructure(fd);
  fd2->ShallowCopy(fd);
  fd2->DeepCopy(fd);

  vtkIdList* ptIds = vtkIdList::New();
  ptIds->InsertNextId(0);
  ptIds->InsertNextId(2);

  fd->GetField(ptIds, fd2);
  ptIds->Delete();

  int arrayComp;
  int a = fd->GetArrayContainingComponent(1, arrayComp);
  if (a != 1)
  {
    return 1;
  }

  /* Obsolete API.
  double tuple[10];
  // initialize tuple before using it to set something
  for (i = 0; i < 10; i++)
    {
    tuple[i] = i;
    }
  fd->GetTuple(2);
  fd->SetTuple(2, tuple);
  fd->InsertTuple(2, tuple);
  fd->InsertNextTuple(tuple);
  fd->SetComponent(0,0, 1.0);
  fd->InsertComponent(0,0, 1.0);
  */
  fd2->Reset();

  fd->Delete();
  fd2->Delete();

  return 0;

}
