/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayAMRVolumeRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test checks if OSPRay based AMR Volume rendering works

#include "vtkAMRGaussianPulseSource.h"
#include "vtkAMRVolumeMapper.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkNew.h"
#include "vtkOSPRayPass.h"
#include "vtkOverlappingAMR.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVolumeProperty.h"

int TestOSPRayAMRVolumeRenderer(int argc, char* argv[])
{
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }

  double scalarRange[2] = { 4.849e-23, 0.4145 };
  vtkNew<vtkAMRVolumeMapper> volumeMapper;

  vtkNew<vtkAMRGaussianPulseSource> amrSource;
  amrSource->SetXPulseOrigin(0);
  amrSource->SetYPulseOrigin(0);
  amrSource->SetZPulseOrigin(0);
  amrSource->SetXPulseWidth(.5);
  amrSource->SetYPulseWidth(.5);
  amrSource->SetZPulseWidth(.5);
  amrSource->SetPulseAmplitude(0.5);
  amrSource->SetDimension(3);
  amrSource->SetRootSpacing(0.5);
  amrSource->SetRefinementRatio(2);
  amrSource->Update();
  volumeMapper->SetInputConnection(amrSource->GetOutputPort());

  volumeMapper->SelectScalarArray("Gaussian-Pulse");
  volumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 0.2);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkColorTransferFunction* colorTransferFunction = volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.8, 0.6, 0.1);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.1, 0.2, 0.8);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // Attach OSPRay render pass
  vtkNew<vtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    ren->SetPass(osprayPass.GetPointer());
  }

  ren->AddViewProp(volume.GetPointer());
  renWin->Render();
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(140);
  ren->GetActiveCamera()->Elevation(30);

  iren->Initialize();
  iren->SetDesiredUpdateRate(30.0);

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  volumeMapper->SetInputConnection(nullptr);
  return !retVal;
}
