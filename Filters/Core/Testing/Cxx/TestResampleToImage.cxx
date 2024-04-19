// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This is just a simple test. vtkResampleToImage internally uses
// vtkProbeFilter, which is tested thoroughly in other tests.

#include "vtkResampleToImage.h"

#include "vtkCell.h"
#include "vtkCellTypeSource.h"
#include "vtkCharArray.h"
#include "vtkClipDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtractVOI.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include <iostream>

int TestResampleToImage(int, char*[])
{
  int status = 0;

  // Create Pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 16, 0, 16, 0, 16);
  wavelet->SetCenter(8, 8, 8);

  vtkNew<vtkClipDataSet> clip;
  clip->SetInputConnection(wavelet->GetOutputPort());
  clip->SetValue(157);

  vtkNew<vtkResampleToImage> resample;
  resample->SetUseInputBounds(true);
  resample->SetSamplingDimensions(32, 32, 32);
  resample->SetInputConnection(clip->GetOutputPort());

  vtkNew<vtkExtractVOI> voi;
  voi->SetVOI(4, 27, 4, 27, 4, 27);
  voi->SetInputConnection(resample->GetOutputPort());
  voi->Update();

  vtkImageData* output = voi->GetOutput();
  vtkIdType numPoints = output->GetNumberOfPoints();
  vtkIdType numCells = output->GetNumberOfCells();
  if (numPoints != 13824 || numCells != 12167)
  {
    std::cout << "Number of points: expecting 13824, got " << numPoints << std::endl;
    std::cout << "Number of cells: expecting 12167, got " << numCells << std::endl;
    status = 1;
  }

  vtkUnsignedCharArray* pointGhostArray = output->GetPointGhostArray();
  vtkIdType numHiddenPoints = 0;
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    if (pointGhostArray->GetValue(i) & vtkDataSetAttributes::HIDDENPOINT)
    {
      ++numHiddenPoints;
    }
  }

  if (numHiddenPoints != 1998)
  {
    std::cout << "Number of Hidden points: expecting 1998 got " << numHiddenPoints << std::endl;
    status = 1;
  }

  vtkUnsignedCharArray* cellGhostArray = output->GetCellGhostArray();
  vtkIdType numHiddenCells = 0;
  for (vtkIdType i = 0; i < numCells; ++i)
  {
    if (cellGhostArray->GetValue(i) & vtkDataSetAttributes::HIDDENCELL)
    {
      ++numHiddenCells;
    }
  }

  if (numHiddenCells != 2169)
  {
    std::cout << "Number of Hidden cells: expecting 2169 got " << numHiddenCells << std::endl;
    status = 1;
  }

  // Test for ParaView issue #19856

  vtkNew<vtkCellTypeSource> cellTypeSource;
  cellTypeSource->SetCellOrder(1);
  cellTypeSource->SetCellType(VTK_HEXAHEDRON);
  cellTypeSource->SetBlocksDimensions(10, 10, 10);

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(7.786580522057762, 5.87458457259413, 6.314673922104045);
  plane->SetNormal(0.8700294812256526, 0.27306383431551773, 0.4104690538726755);

  vtkNew<vtkClipDataSet> clip2;
  clip2->SetInputConnection(cellTypeSource->GetOutputPort());
  clip2->SetClipFunction(plane);

  vtkNew<vtkResampleToImage> resample2;
  resample2->SetInputConnection(clip2->GetOutputPort());
  resample2->SetSamplingDimensions(100, 100, 100);
  resample2->Update();

  // We look for the number of empty voxels
  vtkPointData* cd = vtkDataSet::SafeDownCast(resample2->GetOutputDataObject(0))->GetPointData();
  vtkCharArray* validPoints =
    vtkCharArray::SafeDownCast(cd->GetAbstractArray(resample2->GetMaskArrayName()));
  auto values = vtk::DataArrayValueRange(validPoints);
  vtkIdType zeros = 0;
  for (const auto val : values)
  {
    zeros += !val;
  }
  if (zeros != 744732)
  {
    std::cout << "Caught " << zeros << " invalid points, it should have been 744732" << std::endl;
    status = 1;
  }

  return status;
}
