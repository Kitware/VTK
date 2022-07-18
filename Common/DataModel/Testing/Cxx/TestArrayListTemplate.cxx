/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArrayListTemplate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractArray.h"
#include "vtkArrayListTemplate.h"
#include "vtkCellData.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStringArray.h"

int TestArrayListTemplate(int, char*[])
{
  int retVal = EXIT_SUCCESS;

  vtkNew<vtkRTAnalyticSource> waveletSource;
  waveletSource->SetWholeExtent(0, 1, 0, 1, 0, 1);
  waveletSource->Update();

  vtkNew<vtkStringArray> strings;
  strings->SetNumberOfTuples(1);
  strings->SetValue(0, "foo");
  strings->SetName("Strings");

  vtkImageData* wavelet = vtkImageData::SafeDownCast(waveletSource->GetOutputDataObject(0));
  wavelet->GetCellData()->AddArray(strings);

  // This filter uses vtkArrayListTemplate
  vtkNew<vtkGeometryFilter> filter;
  filter->SetInputData(wavelet);
  filter->Update();

  vtkDataSet* output = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0));

  if (!output->GetCellData()->GetAbstractArray("Strings"))
  {
    vtkLog(ERROR, "vtkStringArray was not passed in vtkGeometryFilter.");
    retVal = EXIT_FAILURE;
  }

  return retVal;
}
