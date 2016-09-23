/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This is just a simple test. vtkResampleToImage internally uses
// vtkProbeFilter, which is tested thoroughly in other tests.

#include "vtkResampleToImage.h"

#include "vtkClipDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtractVOI.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkResampleToImage.h"
#include "vtkRTAnalyticSource.h"
#include "vtkUnsignedCharArray.h"

#include <iostream>


int TestResampleToImage(int , char *[])
{
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

  vtkImageData *output = voi->GetOutput();
  vtkIdType numPoints = output->GetNumberOfPoints();
  vtkIdType numCells = output->GetNumberOfCells();
  if (numPoints != 13824 || numCells != 12167)
  {
    std::cout << "Number of points: expecting 13824, got " << numPoints
              << std::endl;
    std::cout << "Number of cells: expecting 12167, got " << numCells
              << std::endl;
    return 1;
  }

  vtkUnsignedCharArray *pointGhostArray = output->GetPointGhostArray();
  vtkIdType numHiddenPoints = 0;
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    if (pointGhostArray->GetValue(i) & vtkDataSetAttributes::HIDDENPOINT)
    {
      ++numHiddenPoints;
    }
  }

  if (numHiddenPoints != 1855)
  {
    std::cout << "Number of Hidden points: expecting 1855 got "
              << numHiddenPoints << std::endl;
    return 1;
  }

  vtkUnsignedCharArray *cellGhostArray = output->GetCellGhostArray();
  vtkIdType numHiddenCells = 0;
  for (vtkIdType i = 0; i < numCells; ++i)
  {
    if (cellGhostArray->GetValue(i) & vtkDataSetAttributes::HIDDENPOINT)
    {
      ++numHiddenCells;
    }
  }

  if (numHiddenCells != 2054)
  {
    std::cout << "Number of Hidden cells: expecting 2054 got "
              << numHiddenCells << std::endl;
    return 1;
  }

  return 0;
}
