/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBlurAndSobelPasses.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the combination of two post-processing render passes:
// Gaussian blur first, followed by a Sobel detection. It renders of an
// opaque cone.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkActor.h"

#include "vtkImageSinusoidSource.h"
#include "vtkImageData.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkCamera.h"

#include "vtkCameraPass.h"
#include "vtkLightsPass.h"
#include "vtkSequencePass.h"
#include "vtkOpaquePass.h"
#include "vtkDepthPeelingPass.h"
#include "vtkTranslucentPass.h"
#include "vtkVolumetricPass.h"
#include "vtkOverlayPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkGaussianBlurPass.h"
#include "vtkSobelGradientMagnitudePass.h"
#include "vtkConeSource.h"

#include <vtkTestErrorObserver.h>

int TestBlurAndSobelPasses(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);

  vtkOpenGLRenderer *glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);

  vtkSmartPointer<vtkCameraPass> cameraP =
    vtkSmartPointer<vtkCameraPass>::New();

  vtkSmartPointer<vtkSequencePass> seq =
    vtkSmartPointer<vtkSequencePass>::New();
  vtkSmartPointer<vtkOpaquePass> opaque =
    vtkSmartPointer<vtkOpaquePass>::New();
  vtkSmartPointer<vtkDepthPeelingPass> peeling =
    vtkSmartPointer<vtkDepthPeelingPass>::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  vtkSmartPointer<vtkTranslucentPass> translucent =
    vtkSmartPointer<vtkTranslucentPass>::New();
  peeling->SetTranslucentPass(translucent);

  vtkSmartPointer<vtkVolumetricPass> volume =
    vtkSmartPointer<vtkVolumetricPass>::New();
  vtkSmartPointer<vtkOverlayPass> overlay =
    vtkSmartPointer<vtkOverlayPass>::New();

  vtkSmartPointer<vtkLightsPass> lights =
    vtkSmartPointer<vtkLightsPass>::New();

  vtkSmartPointer<vtkRenderPassCollection> passes =
    vtkSmartPointer<vtkRenderPassCollection>::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);

  passes->AddItem(peeling);
//  passes->AddItem(translucent);

  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);


  vtkSmartPointer<vtkGaussianBlurPass> blurP =
    vtkSmartPointer<vtkGaussianBlurPass>::New();
  blurP->SetDelegatePass(cameraP);

  vtkSmartPointer<vtkTest::ErrorObserver> errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkSobelGradientMagnitudePass> sobelP =
    vtkSmartPointer<vtkSobelGradientMagnitudePass>::New();
  sobelP->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  sobelP->SetDelegatePass(blurP);
  if (!errorObserver->GetError())
  {
    std::cout << "The required extensions are not supported."  << std::endl;
    return 0;
  }

  glrenderer->SetPass(sobelP);

//  renderer->SetPass(cameraP);

  vtkSmartPointer<vtkImageSinusoidSource> imageSource =
    vtkSmartPointer<vtkImageSinusoidSource>::New();
  imageSource->SetWholeExtent(0,9,0,9,0,9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  vtkImageData *image=imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  vtkSmartPointer<vtkDataSetSurfaceFilter> surface =
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();

  surface->SetInputConnection(imageSource->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(surface->GetOutputPort());

  vtkSmartPointer<vtkLookupTable> lut =
    vtkSmartPointer<vtkLookupTable>::New();
  lut->SetTableRange(range);
  lut->SetAlphaRange(0.5,0.5);
  lut->SetHueRange(0.2,0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  actor->SetVisibility(0);

  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> coneMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());
  coneMapper->SetImmediateModeRendering(1);
  vtkSmartPointer<vtkActor> coneActor =
    vtkSmartPointer<vtkActor>::New();
  coneActor->SetMapper(coneMapper);
  coneActor->SetVisibility(1);
  renderer->AddActor(coneActor);

  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(400,400);

  renWin->Render();
  if(peeling->GetLastRenderingUsedDepthPeeling())
  {
    cout<<"depth peeling was used"<<endl;
  }
  else
  {
    cout<<"depth peeling was not used (alpha blending instead)"<<endl;
  }
  vtkCamera *camera=renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
