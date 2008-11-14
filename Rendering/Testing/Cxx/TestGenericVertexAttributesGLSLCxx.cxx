/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericVertexAttributesGLSLCxx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkGLSLShaderDeviceAdapterCxx
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

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestGenericVertexAttributesGLSLCxx(int argc, char *argv[])
{
  char shaders[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
  <Material name=\"GenericAttributes1\"> \
    <Shader scope=\"Vertex\" name=\"VertexShader\" location=\"Inline\"\
      language=\"GLSL\" entry=\"main\"> attribute vec3 genAttrVector; \
      varying vec4 color; \
      void main(void) \
      { \
        gl_Position = gl_ModelViewProjectionMatrix *gl_Vertex; \
        color = vec4(normalize(genAttrVector), 1.0); \
      } \
    </Shader> \
    <Shader scope=\"Fragment\" name=\"FragmentShader\" location=\"Inline\" \
      language=\"GLSL\" entry=\"main\"> \
      varying vec4 color; \
      void main(void) \
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
  actor->GetProperty()->LoadMaterialFromString(shaders);
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actor->GetProperty()->GetShading();
  actor->GetProperty()->ShadingOn();

  mapper->MapDataArrayToVertexAttribute("genAttrVector", "BrownianVectors", 0, -1);

  vtkRenderer *renderer = vtkRenderer::New();
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->SetSize(400,400);
  renWin->Render();
  interactor->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin,18);
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    interactor->Start();
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
