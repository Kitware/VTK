/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalSupport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPolyData.h"
#include "vtkNew.h"
#include "vtkPolyDataNormals.h"

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

int TestSetInputDataObject(int, char*[])
{
  vtkNew<vtkPolyData> inputData;
  vtkNew<vtkPolyData> inputData2;

  vtkNew<vtkPolyDataNormals> filter;

  unsigned long mtime = filter->GetMTime();

  // this should change the filter's mtime.
  filter->SetInputDataObject(inputData.GetPointer());

  unsigned long changed_mtime = filter->GetMTime();
  if (changed_mtime <= mtime)
    {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() did not change the Mtime!!!" << endl;
    return TEST_FAILURE;
    }

  // this should *not* change the filter's mtime.
  filter->SetInputDataObject(inputData.GetPointer());
  if (changed_mtime != filter->GetMTime())
    {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() changed the Mtime!!!" << endl;
    return TEST_FAILURE;
    }

  // this should change the filter's mtime.
  filter->SetInputDataObject(inputData2.GetPointer());
  if (filter->GetMTime() <= changed_mtime)
    {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() did not change the Mtime!!!" << endl;
    return TEST_FAILURE;
    }

  changed_mtime = filter->GetMTime();

  // this should change the filter's mtime.
  filter->SetInputDataObject(NULL);
  if (filter->GetMTime() <= changed_mtime)
    {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() did not change the Mtime!!!" << endl;
    return TEST_FAILURE;
    }

  changed_mtime = filter->GetMTime();

  // this should *not* change the filter's mtime.
  filter->SetInputDataObject(NULL);
  if (filter->GetMTime() != changed_mtime)
    {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() changed the Mtime!!!" << endl;
    return TEST_FAILURE;
    }

  return TEST_SUCCESS;
}
