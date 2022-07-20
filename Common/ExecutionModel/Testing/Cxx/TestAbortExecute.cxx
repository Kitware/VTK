/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAbortExecute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClipDataSet.h"
#include "vtkContourGrid.h"
#include "vtkInformation.h"
#include "vtkPlane.h"
#include "vtkRTAnalyticSource.h"
#include "vtkShrinkFilter.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>
#include <cstring>

int TestAbortExecute(int, char*[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkShrinkFilter> shrink;
  vtkNew<vtkContourGrid> contour;
  vtkNew<vtkClipDataSet> clip;

  wavelet->SetWholeExtent(0, 10, 0, 10, 0, 10);

  shrink->SetInputConnection(wavelet->GetOutputPort());

  contour->SetInputConnection(shrink->GetOutputPort());
  contour->GenerateValues(1, 10, 10);

  vtkNew<vtkPlane> clipPlane;
  clipPlane->SetNormal(1, 0, 0);
  clipPlane->SetOrigin(0, 0, 0);

  clip->SetInputConnection(contour->GetOutputPort());
  clip->SetClipFunction(clipPlane);

  wavelet->SetAbortExecuteAndUpdateTime();
  clip->Update();

  if (!wavelet->GetAbortExecute())
  {
    std::cerr << "Wavelet AbortExecute flag is not set." << std::endl;
    return 1;
  }

  if (shrink->GetAbortExecute() || contour->GetAbortExecute() || clip->GetAbortExecute())
  {
    std::cerr << "Shrink, Contour, or Clip AbortExecute flag is set." << std::endl;
    return 1;
  }

  if (!wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    std::cerr << "Wavelet, Shrink, Contour, or Clip ABORTED flag is not set." << std::endl;
    return 1;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    std::cerr << "Found output data." << std::endl;
    return 1;
  }

  wavelet->SetAbortExecute(0);
  shrink->SetAbortExecuteAndUpdateTime();
  clip->Update();

  if (!shrink->GetAbortExecute())
  {
    std::cerr << "Shrink AbortExecute flag is not set." << std::endl;
    return 1;
  }

  if (wavelet->GetAbortExecute() || contour->GetAbortExecute() || clip->GetAbortExecute())
  {
    std::cerr << "Wavelet, Contour, or Clip AbortExecute flag is set." << std::endl;
    return 1;
  }

  if (wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    std::cerr << "Wavelet ABORTED flag is set." << std::endl;
    return 1;
  }

  if (!shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    !clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    std::cerr << "Wavelet, Shrink, Contour, or Clip ABORTED flag is not set." << std::endl;
    return 1;
  }

  if (clip->GetOutput()->GetNumberOfPoints())
  {
    std::cerr << "Found output data." << std::endl;
    return 1;
  }

  shrink->SetAbortExecute(0);
  clip->Update();

  if (wavelet->GetAbortExecute() || shrink->GetAbortExecute() || contour->GetAbortExecute() ||
    clip->GetAbortExecute())
  {
    std::cerr << "Wavelet, Shrink, Contour, or Clip AbortExecute flag is set." << std::endl;
    return 1;
  }

  if (wavelet->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    shrink->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    contour->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()) ||
    clip->GetOutputInformation(0)->Get(vtkAlgorithm::ABORTED()))
  {
    std::cerr << "Wavelet, Shrink, Contour, or Clip ABORTED flag is set." << std::endl;
    return 1;
  }

  if (!clip->GetOutput()->GetNumberOfPoints())
  {
    std::cerr << "No output data." << std::endl;
    return 1;
  }

  return 0;
}
