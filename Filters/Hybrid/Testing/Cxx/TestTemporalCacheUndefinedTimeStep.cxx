// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCompositeDataGeometryFilter.h>
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkTemporalDataSetCache.h>
#include <vtkTestUtilities.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <iostream>
int TestTemporalCacheUndefinedTimeStep(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  std::string fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fileName.c_str());

  vtkNew<vtkTemporalDataSetCache> temporalCache;
  temporalCache->SetInputConnection(reader->GetOutputPort());
  temporalCache->SetCacheSize(43);

  // Reduce size of output
  vtkNew<vtkCompositeDataGeometryFilter> geometryFilter;
  geometryFilter->SetInputConnection(temporalCache->GetOutputPort());
  geometryFilter->UpdateTimeStep(0.00165); // Doesn't exist

  return vtkTestUtilities::RegressionTest(argc, argv, geometryFilter->GetOutputDataObject(0),
           "/Data/BaselineData/TestTemporalCacheUndefinedTimeStep.vtkhdf")
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
