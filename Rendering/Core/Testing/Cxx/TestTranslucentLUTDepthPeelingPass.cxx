/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTranslucentLUTDepthPeelingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers rendering of an actor with a translucent LUT and depth
// peeling using the multi renderpass classes. The mapper uses color
// interpolation (poor quality).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
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

#include "vtkgl.h"

// Make sure to have a valid OpenGL context current on the calling thread
// before calling it.
bool MesaHasVTKBug8135(vtkRenderWindow *renwin)
{
  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(renwin);

  vtkOpenGLExtensionManager *extmgr
    = context->GetExtensionManager();

  return (extmgr->DriverIsMesa() && !extmgr->DriverVersionAtLeast(7,3));
}

int TestTranslucentLUTDepthPeelingPass(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  vtkOpenGLRenderer *glrenderer = vtkOpenGLRenderer::SafeDownCast(renderer);

  vtkCameraPass *cameraP=vtkCameraPass::New();

  vtkSequencePass *seq=vtkSequencePass::New();
  vtkOpaquePass *opaque=vtkOpaquePass::New();
  vtkDepthPeelingPass *peeling=vtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);

  vtkTranslucentPass *translucent=vtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);

  vtkVolumetricPass *volume=vtkVolumetricPass::New();
  vtkOverlayPass *overlay=vtkOverlayPass::New();

  vtkLightsPass *lights=vtkLightsPass::New();

  vtkRenderPassCollection *passes=vtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
  passes->AddItem(peeling);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);
  glrenderer->SetPass(cameraP);

  opaque->Delete();
  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();

  vtkImageSinusoidSource *imageSource=vtkImageSinusoidSource::New();
  imageSource->SetWholeExtent(0,9,0,9,0,9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  vtkImageData *image=imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  vtkDataSetSurfaceFilter *surface=vtkDataSetSurfaceFilter::New();

  surface->SetInputConnection(imageSource->GetOutputPort());
  imageSource->Delete();

  vtkPolyDataMapper *mapper=vtkPolyDataMapper::New();
  mapper->SetInputConnection(surface->GetOutputPort());
  surface->Delete();

  vtkLookupTable *lut=vtkLookupTable::New();
  lut->SetTableRange(range);
  lut->SetAlphaRange(0.5,0.5);
  lut->SetHueRange(0.2,0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);
  lut->Delete();

  vtkActor *actor=vtkActor::New();
  renderer->AddActor(actor);
  actor->Delete();
  actor->SetMapper(mapper);
  mapper->Delete();

  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(400,400);

  // empty scene during OpenGL detection.
  actor->SetVisibility(0);
  renWin->Render();

  int retVal;
  if(MesaHasVTKBug8135(renWin))
    {
    // Mesa will crash if version<7.3
    cout<<"This version of Mesa would crash. Skip the test."<<endl;
    retVal=vtkRegressionTester::PASSED;
    }
  else
    {
    actor->SetVisibility(1);
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
  iren->Delete();

  return !retVal;
}
