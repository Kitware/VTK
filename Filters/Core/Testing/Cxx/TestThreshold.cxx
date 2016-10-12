/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkNew.h"
#include "vtkThreshold.h"
#include "vtkRTAnalyticSource.h"
#include "vtkDataObject.h"
#include "vtkUnstructuredGrid.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
int TestThreshold(int, char *[])
{
  //---------------------------------------------------
  // Test using different thresholding methods
  //---------------------------------------------------
  vtkNew<vtkRTAnalyticSource> source;
  vtkNew<vtkThreshold> filter;
  filter->SetInputConnection(source->GetOutputPort());

  double L=100;
  double U=200;
  filter->ThresholdBetween(L,U);
  filter->SetAllScalars(0);
  filter->Update();
  int n1 = filter->GetOutput()->GetNumberOfCells();

  filter->UseContinuousCellRangeOn();
  filter->Update();
  int n2 = filter->GetOutput()->GetNumberOfCells();

  //we we are using a large query range,
  //whether to use continuous range or not should not matter
  if(n1!=n2)
  {
    return EXIT_FAILURE;
  }

  filter->UseContinuousCellRangeOff();
  filter->ThresholdBetween(L,L);
  filter->Update();
  //since we are not using continuous cell range
  //no cell points should fall in the empty interval
  if(filter->GetOutput()->GetNumberOfCells()>0)
  {
    return EXIT_FAILURE;
  }
  filter->UseContinuousCellRangeOn();
  filter->Update();
  if(filter->GetOutput()->GetNumberOfCells()==0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
