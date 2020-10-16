/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumeGhostArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test GPU ray cast support for ghost point and cell blanking

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSetAttributes.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastVolumeGhostArrays(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-127, 128, -127, 128, -127, 128);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  wavelet->Update();

  vtkImageData* im = vtkImageData::SafeDownCast(wavelet->GetOutput());
  int dims[3];
  im->GetDimensions(dims);
  im->AllocatePointGhostArray();
  im->AllocateCellGhostArray();

  vtkUnsignedCharArray* ptGhostArr = im->GetPointGhostArray();
  vtkUnsignedCharArray* cellGhostArr = im->GetCellGhostArray();

  // Create some hidden points and cells
  for (int k = 0; k < (dims[2] - 1); ++k)
  {
    for (int j = (dims[1] - 1) / 4; j < 3 * (dims[1] - 1) / 4; ++j)
    {
      for (int i = (dims[0] - 1) / 3; i < 2 * (dims[0] - 1) / 3; ++i)
      {
        // Current cell Id
        int cellId = (k * (dims[0] - 1) + j) * (dims[0] - 1) + i;
        // Set the first set of cells as hidden cells
        int flag = vtkDataSetAttributes::HIDDENCELL;
        if (i > (dims[0] - 1) / 2 && k < (dims[2] - 1) / 2)
        {
          // Set the next set of cells as duplicate
          flag = vtkDataSetAttributes::DUPLICATECELL;
        }
        else if (k > (dims[2] - 1) / 2)
        {
          // Set the last set as refined
          flag = vtkDataSetAttributes::REFINEDCELL;
        }
        cellGhostArr->SetValue(cellId, flag);
      }
      for (int i = 3 * dims[0] / 4; i < dims[0]; ++i)
      {
        int ptId = (k * dims[0] + j) * dims[1] + i;
        // Set the first set of points as hidden
        int flag = vtkDataSetAttributes::HIDDENPOINT;
        if (j > (dims[1] - 1) / 2)
        {
          flag = vtkDataSetAttributes::DUPLICATEPOINT;
        }
        if (k < (dims[2] - 1) / 2)
        {
          ptGhostArr->SetValue(ptId, flag);
        }
      }
    }
  }

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputData(im);

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.2, 0.29, 1);
  ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  ctf->AddRGBPoint(276.829, 0.7, 0.015, 0.15);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 0.0);
  pwf->AddPoint(276.829, 0.03);

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pwf);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->Render(); // make sure we have an OpenGL context.

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);
  auto camera = renderer->GetActiveCamera();
  camera->SetPosition(0, 0, 0);
  camera->SetFocalPoint(-0.1, 0.2, 1);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  return vtkTesting::InteractorEventLoop(argc, argv, iren);
}
