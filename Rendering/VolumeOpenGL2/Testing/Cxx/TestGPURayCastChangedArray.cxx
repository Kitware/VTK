/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastChangedArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Designed to test paraview/paraview#19012: when the array to volume render
 * with is changed, the volume mapper must update correctly.
 */

#include <vtkArrayCalculator.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkProperty.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkTestingObjectFactory.h>
#include <vtkVolumeProperty.h>

int TestGPURayCastChangedArray(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> rtSource;
  rtSource->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkArrayCalculator> calculator;
  calculator->SetInputConnection(rtSource->GetOutputPort());
  calculator->AddScalarArrayName("RTData");
  calculator->SetResultArrayName("sin_RTData");
  calculator->SetFunction("100*sin(RTData)");

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(calculator->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectScalarArray("sin_RTData");

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(250.0, 1.0, 1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(0, 0.0);
  scalarOpacity->AddPoint(250, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  iren->SetInteractorStyle(style);

  // render using sin_RTData.
  mapper->SelectScalarArray("sin_RTData");
  renderWindow->Render();
  renderer->ResetCamera();

  // change array and re-render.
  mapper->SelectScalarArray("RTData");
  renderWindow->Render();

  iren->Initialize();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
