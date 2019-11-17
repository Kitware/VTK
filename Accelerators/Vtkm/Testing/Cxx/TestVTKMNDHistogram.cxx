/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVTKMClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkmNDHistogram.h"

#include "vtkActor.h"
#include "vtkArrayData.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDelaunay3D.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageToPoints.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkSparseArray.h"
#include "vtkTable.h"

namespace
{
static const std::vector<std::string> arrayNames = { "temperature0", "temperature1", "temperature2",
  "temperature3" };
static const std::vector<std::vector<size_t> > resultBins = {
  { 0, 0, 1, 1, 2, 2, 3, 3 },
  { 0, 1, 1, 2, 2, 3, 3, 4 },
  { 0, 1, 2, 2, 3, 4, 4, 5 },
  { 0, 1, 2, 3, 3, 4, 5, 6 },
};
static const std::vector<size_t> resultFrequency = { 2, 1, 1, 1, 1, 1, 1, 2 };
static const int nData = 10;
static const std::vector<size_t> bins = { 4, 5, 6, 7 };
void AddArrayToVTKData(
  std::string scalarName, vtkDataSetAttributes* pd, double* data, vtkIdType size)
{
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetArray(data, size, 1);
  scalars->SetName(scalarName.c_str());
  pd->AddArray(scalars);
}

void MakeTestDataset(vtkDataSet* dataset)
{
  static double T0[nData], T1[nData], T2[nData], T3[nData];
  for (int i = 0; i < nData; i++)
  {
    T0[i] = i * 1.0;
    T1[i] = i * 2.0;
    T2[i] = i * 3.0;
    T3[i] = i * 4.0;
  }

  vtkPointData* pd = dataset->GetPointData();
  AddArrayToVTKData(arrayNames[0], pd, T0, static_cast<vtkIdType>(nData));
  AddArrayToVTKData(arrayNames[1], pd, T1, static_cast<vtkIdType>(nData));
  AddArrayToVTKData(arrayNames[2], pd, T2, static_cast<vtkIdType>(nData));
  AddArrayToVTKData(arrayNames[3], pd, T3, static_cast<vtkIdType>(nData));
}
}

int TestVTKMNDHistogram(int, char*[])
{
  vtkNew<vtkPolyData> ds;
  MakeTestDataset(ds);

  vtkNew<vtkmNDHistogram> filter;
  filter->SetInputData(ds);
  size_t index = 0;
  for (const auto& an : arrayNames)
  {
    filter->AddFieldAndBin(an, bins[index++]);
  }
  filter->Update();
  vtkArrayData* arrayData = filter->GetOutput();

  assert(arrayData != nullptr);
  // Valid the data range and bin delta
  for (vtkIdType i = 0; i < 4; i++)
  {
    // Validate the delta and range
    auto range = filter->GetDataRange(i);
    double delta = filter->GetBinDelta(i);
    if (range.first != 0.0 || range.second != (i + 1) * 9)
    {
      std::cout << "array index=" << i << " does not have right range" << std::endl;
      return 1;
    }
    if (delta != ((range.second - range.first) / bins[i]))
    {
      std::cout << "array index" << i << " does not have right delta" << std::endl;
      return 1;
    }
  }
  vtkSparseArray<double>* sa = static_cast<vtkSparseArray<double>*>(arrayData->GetArray(0));
  vtkArrayCoordinates coordinates;
  const vtkIdType dimensions = sa->GetDimensions();     // 4
  const vtkIdType non_null_size = sa->GetNonNullSize(); // 8
  for (vtkIdType n = 0; n != non_null_size; ++n)
  {
    sa->GetCoordinatesN(n, coordinates);
    for (vtkIdType d = 0; d != dimensions; ++d)
    {
      assert(coordinates[d] == static_cast<vtkIdType>(resultBins[d][n]));
      if (coordinates[d] != static_cast<vtkIdType>(resultBins[d][n]))
      {
        std::cout << "value does not match at index " << n << " dimension " << d << std::endl;
      }
    }
    assert(resultFrequency[n] == sa->GetValue(coordinates));
  }
  return 0;
}
