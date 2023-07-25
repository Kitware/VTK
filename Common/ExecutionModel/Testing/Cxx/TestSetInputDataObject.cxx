// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

int TestSetInputDataObject(int, char*[])
{
  vtkNew<vtkPolyData> inputData;
  vtkNew<vtkPolyData> inputData2;

  vtkNew<vtkPolyDataNormals> filter;

  vtkMTimeType mtime = filter->GetMTime();

  // this should change the filter's mtime.
  filter->SetInputDataObject(inputData);

  vtkMTimeType changed_mtime = filter->GetMTime();
  if (changed_mtime <= mtime)
  {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() did not change the Mtime!!!" << endl;
    return TEST_FAILURE;
  }

  // this should *not* change the filter's mtime.
  filter->SetInputDataObject(inputData);
  if (changed_mtime != filter->GetMTime())
  {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() changed the Mtime!!!" << endl;
    return TEST_FAILURE;
  }

  // this should change the filter's mtime.
  filter->SetInputDataObject(inputData2);
  if (filter->GetMTime() <= changed_mtime)
  {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() did not change the Mtime!!!" << endl;
    return TEST_FAILURE;
  }

  changed_mtime = filter->GetMTime();

  // this should change the filter's mtime.
  filter->SetInputDataObject(nullptr);
  if (filter->GetMTime() <= changed_mtime)
  {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() did not change the Mtime!!!" << endl;
    return TEST_FAILURE;
  }

  changed_mtime = filter->GetMTime();

  // this should *not* change the filter's mtime.
  filter->SetInputDataObject(nullptr);
  if (filter->GetMTime() != changed_mtime)
  {
    cerr << __LINE__ << ": ERROR: SetInputDataObject() changed the Mtime!!!" << endl;
    return TEST_FAILURE;
  }

  return TEST_SUCCESS;
}
