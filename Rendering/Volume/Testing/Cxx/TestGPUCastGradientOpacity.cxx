/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastGradientOpacity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGPUVolumeRayCastMapper.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStructuredPointsReader.h"
#include "vtkSLCReader.h"
#include "vtkStructuredPoints.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include <vtkNew.h>
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkContourFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkCullerCollection.h"
#include "vtkCuller.h"
#include "vtkFrustumCoverageCuller.h"
#include "vtkVolumeTextureMapper3D.h"
#include <vtkXMLImageDataReader.h>
#include <vtkFixedPointVolumeRayCastMapper.h>

//#define USE_TEXTURE_MAPPER_3D

int TestGPUCastGradientOpacity(int argc, char *argv[])
{
  // Create the standard renderer, render window, and interactor.
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  ren1->Delete();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  iren->SetDesiredUpdateRate(3);

  // Create the reader for the data.
  // This is the data that will be volume rendered.
  vtkXMLImageDataReader *reader = vtkXMLImageDataReader::New();
  char *cfname=vtkTestUtilities::ExpandDataFileName(argc,argv,"Data/Torso.vti");
  std::cerr << "cfname " << cfname << std::endl;
  reader->SetFileName(cfname);
  delete[] cfname;

#if 0
  vtkFixedPointVolumeRayCastMapper* volumeMapper= vtkFixedPointVolumeRayCastMapper::New();
#else
  vtkGPUVolumeRayCastMapper* volumeMapper= vtkGPUVolumeRayCastMapper::New();
#endif
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  vtkVolume *volume=vtkVolume::New();
  volume->SetMapper(volumeMapper);

  // Create gradient opacity function
  vtkPiecewiseFunction* scalarFunction = vtkPiecewiseFunction::New();
  scalarFunction->AddPoint(-680, 0.0);
  scalarFunction->AddPoint(0, 0.03);
  scalarFunction->AddPoint(300, 0.2);

  vtkColorTransferFunction* colorFunction = vtkColorTransferFunction::New();
  colorFunction->AddHSVPoint(-1024, 0.09, 0.33, 0.82);
  colorFunction->AddHSVPoint(-330, 0.09, 0.33, 0.82);
  colorFunction->AddRGBPoint(100, 1.0, 1.0, 1.0);
  colorFunction->AddRGBPoint(3072, 1.0, 1.0, 1.0);

  vtkPiecewiseFunction* gradientOpacityFunction = vtkPiecewiseFunction::New();
  gradientOpacityFunction->AddPoint(50, 0.0);
  gradientOpacityFunction->AddPoint(100, 1.0);

  vtkVolumeProperty* volumeProperty = volume->GetProperty();
  volumeProperty->SetGradientOpacity(0, gradientOpacityFunction);
  volumeProperty->SetDisableGradientOpacity(0);
  volumeProperty->SetScalarOpacity(scalarFunction);
  volumeProperty->SetColor(0, colorFunction);
  volumeProperty->SetAmbient(0, 0.0);
  volumeProperty->SetDiffuse(0, 0.5);
  volumeProperty->SetSpecular(0, 1.0);
  volumeProperty->ShadeOn();

  ren1->AddViewProp(volume);
  ren1->SetBackground(0.1,0.2,0.4);
  renWin->SetSize(1000, 1000);
  renWin->Render();
  ren1->ResetCamera();
  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  // Clean up.
  iren->Delete();
}

