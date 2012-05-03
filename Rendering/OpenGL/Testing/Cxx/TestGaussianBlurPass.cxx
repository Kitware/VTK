/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGaussianBlurPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the gaussian blur post-processing render pass.
// It renders an actor with a translucent LUT and depth
// peeling using the multi renderpass classes. The mapper uses color
// interpolation (poor quality).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkSmartPointer.h"
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
#include "vtkConeSource.h"

// Make sure to have a valid OpenGL context current on the calling thread
// before calling it. Defined in TestGenericVertexAttributesGLSLAlphaBlending.
bool MesaHasVTKBug8135();

int TestGaussianBlurPass(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren=
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);

  vtkOpenGLRenderer *glrenderer =
      vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());

  vtkSmartPointer<vtkCameraPass> cameraP=
    vtkSmartPointer<vtkCameraPass>::New();

  vtkSmartPointer<vtkSequencePass> seq=
    vtkSmartPointer<vtkSequencePass>::New();
  vtkSmartPointer<vtkOpaquePass> opaque=
    vtkSmartPointer<vtkOpaquePass>::New();
  vtkSmartPointer<vtkDepthPeelingPass> peeling=
    vtkSmartPointer<vtkDepthPeelingPass>::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  vtkSmartPointer<vtkTranslucentPass> translucent=
    vtkSmartPointer<vtkTranslucentPass>::New();
  peeling->SetTranslucentPass(translucent);

  vtkSmartPointer<vtkVolumetricPass> volume=
    vtkSmartPointer<vtkVolumetricPass>::New();
  vtkSmartPointer<vtkOverlayPass> overlay=
    vtkSmartPointer<vtkOverlayPass>::New();

  vtkSmartPointer<vtkLightsPass> lights=
    vtkSmartPointer<vtkLightsPass>::New();

  vtkSmartPointer<vtkRenderPassCollection> passes=
    vtkSmartPointer<vtkRenderPassCollection>::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);

  passes->AddItem(peeling);
//  passes->AddItem(translucent);

  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);

  vtkSmartPointer<vtkGaussianBlurPass> blurP=
    vtkSmartPointer<vtkGaussianBlurPass>::New();
  blurP->SetDelegatePass(cameraP);

  glrenderer->SetPass(blurP);

//  renderer->SetPass(cameraP);

  vtkSmartPointer<vtkImageSinusoidSource> imageSource=
    vtkSmartPointer<vtkImageSinusoidSource>::New();
  imageSource->SetWholeExtent(0,9,0,9,0,9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  vtkImageData *image=imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  vtkSmartPointer<vtkDataSetSurfaceFilter> surface=
    vtkSmartPointer<vtkDataSetSurfaceFilter>::New();

  surface->SetInputConnection(imageSource->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> mapper=
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(surface->GetOutputPort());

  vtkSmartPointer<vtkLookupTable> lut=
    vtkSmartPointer<vtkLookupTable>::New();
  lut->SetTableRange(range);
  lut->SetAlphaRange(0.5,0.5);
  lut->SetHueRange(0.2,0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);

  vtkSmartPointer<vtkActor> actor=
    vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  actor->SetVisibility(1);

  vtkSmartPointer<vtkConeSource> cone=
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> coneMapper=
    vtkSmartPointer<vtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());
  coneMapper->SetImmediateModeRendering(1);

  vtkSmartPointer<vtkActor> coneActor=
    vtkSmartPointer<vtkActor>::New();
  coneActor->SetMapper(coneMapper);
  coneActor->SetVisibility(1);

  renderer->AddActor(coneActor);

  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(400,400);

  // empty scene during OpenGL detection.
  actor->SetVisibility(0);
  coneActor->SetVisibility(0);
  renWin->Render();

  int retVal;
  if(MesaHasVTKBug8135())
    {
    // Mesa will crash if version<7.3
    cout<<"This version of Mesa would crash. Skip the test."<<endl;
    retVal=vtkRegressionTester::PASSED;
    }
  else
    {
    actor->SetVisibility(1);
    coneActor->SetVisibility(1);
    renderer->ResetCamera();
    vtkCamera *camera=renderer->GetActiveCamera();
    camera->Azimuth(-40.0);
    camera->Elevation(20.0);
    renWin->Render();

    if(peeling->GetLastRenderingUsedDepthPeeling())
      {
      cout<<"depth peeling was used"<<endl;
      }
    else
      {
      cout<<"depth peeling was not used (alpha blending instead)"<<endl;
      }

    retVal = vtkRegressionTestImage( renWin );
    if ( retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      }
    }
  return !retVal;
}
