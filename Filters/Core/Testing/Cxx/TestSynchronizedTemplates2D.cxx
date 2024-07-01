// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSynchronizedTemplates2D.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkXMLPolyDataReader.h"

int TestSynchronizedTemplates2D(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  const std::string dataRoot = testHelper->GetDataRoot();
  const std::string baselineFileName = dataRoot + "/Data/SynchronizedTemplates2D.vtp";

  vtkNew<vtkRTAnalyticSource> waveletSource;
  waveletSource->SetWholeExtent(-10, 10, -10, 10, 0, 0);
  waveletSource->SetCenter(0.0, 0.0, 0.0);
  waveletSource->Update();

  vtkImageData* wavelet = waveletSource->GetOutput();

  vtkNew<vtkSynchronizedTemplates2D> stFilter;
  stFilter->SetInputData(wavelet);
  stFilter->GenerateValues(3, 100, 250);
  stFilter->Update();

  // Test the filter when input has no array set as "Scalars".
  vtkSmartPointer<vtkDataArray> rtData =
    vtk::MakeSmartPointer(wavelet->GetPointData()->GetScalars());
  wavelet->GetPointData()->Initialize();
  wavelet->GetPointData()->AddArray(rtData);
  stFilter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, rtData->GetName());
  stFilter->Update();

  vtkPolyData* outputData = stFilter->GetOutput();

  vtkNew<vtkXMLPolyDataReader> baselineReader;
  baselineReader->SetFileName(baselineFileName.c_str());
  baselineReader->Update();
  vtkPolyData* baseline = baselineReader->GetOutput();

  vtkTestUtilities::CompareDataObjects(outputData, baseline);

  return EXIT_SUCCESS;
}
