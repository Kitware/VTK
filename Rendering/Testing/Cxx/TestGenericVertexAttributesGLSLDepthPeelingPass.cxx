/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericVertexAttributesGLSLDepthPeelingPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkGLSLShaderDeviceAdapter2 with XML shader style 2 with
// depth peeling pass.
// .SECTION Description
// this program tests the shader support in vtkRendering.


#include "vtkActor.h"
#include "vtkBrownianPoints.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"

#include "vtkCameraPass.h"
#include "vtkLightsPass.h"
#include "vtkSequencePass.h"
#include "vtkOpaquePass.h"
#include "vtkDepthPeelingPass.h"
#include "vtkTranslucentPass.h"
#include "vtkVolumetricPass.h"
#include "vtkOverlayPass.h"
#include "vtkRenderPassCollection.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkgl.h"

// Make sure to have a valid OpenGL context current on the calling thread
// before calling it. Defined in TestGenericVertexAttributesGLSLAlphaBlending.
bool MesaHasVTKBug8135();

int TestGenericVertexAttributesGLSLDepthPeelingPass(int argc, char *argv[])
{
  char shaders1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
  <Material name=\"GenericAttributes1\"> \
    <Shader scope=\"Vertex\" name=\"VertexShader\" location=\"Inline\"\
      language=\"GLSL\" entry=\"main\" style=\"2\"> attribute vec3 genAttrVector; \
      varying vec4 color; \
      void propFuncVS(void) \
      { \
        gl_Position = gl_ModelViewProjectionMatrix *gl_Vertex; \
        color = vec4(normalize(genAttrVector), 0.3); \
      } \
    </Shader> \
    <Shader scope=\"Fragment\" name=\"FragmentShader\" location=\"Inline\" \
      language=\"GLSL\" entry=\"main\" style=\"2\"> \
      varying vec4 color; \
      void propFuncFS() \
      { \
        gl_FragColor = color; \
      } \
    </Shader> \
  </Material>";
  
  vtkSphereSource * sphere = vtkSphereSource::New();
  sphere->SetRadius(5);
  sphere->SetPhiResolution(20);
  sphere->SetThetaResolution(20);

  vtkBrownianPoints * randomVector = vtkBrownianPoints::New();
  randomVector->SetMinimumSpeed(0);
  randomVector->SetMaximumSpeed(1);
  randomVector->SetInputConnection(sphere->GetOutputPort());

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(randomVector->GetOutputPort());

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->LoadMaterialFromString(shaders1);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actor->GetProperty()->GetShading();
  actor->GetProperty()->ShadingOn();
  actor->GetProperty()->SetOpacity(0.99); // to force depth peeling.

  mapper->MapDataArrayToVertexAttribute("genAttrVector", "BrownianVectors", 0, -1);

  vtkRenderer *renderer = vtkRenderer::New();
  renderer->SetBackground(0.5, 0.5, 0.5);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  
  // All the passes.
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
  renderer->SetPass(cameraP);
  
  opaque->Delete();
  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();

  
  vtkRenderWindowInteractor *interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->SetSize(400,400);
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
    renderer->AddActor(actor);
    renderer->ResetCamera();
    renWin->Render();
    
    if(peeling->GetLastRenderingUsedDepthPeeling())
      {
      cout<<"depth peeling was used"<<endl;
      }
    else
      {
      cout<<"depth peeling was not used (alpha blending instead)"<<endl;
      }
    interactor->Initialize();
    renWin->Render();
    
    retVal = vtkRegressionTestImageThreshold(renWin,18);
    if( retVal == vtkRegressionTester::DO_INTERACTOR)
      {
      interactor->Start();
      }
    }

  sphere->Delete();
  randomVector->Delete();
  mapper->Delete();
  actor->Delete();
  renderer->Delete();
  renWin->Delete();
  interactor->Delete();

  return !retVal;
}
