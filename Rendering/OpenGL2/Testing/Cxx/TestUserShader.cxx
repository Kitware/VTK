/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkActor.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkPolyDataNormals.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestUserShader(int argc, char *argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                               "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  vtkNew<vtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());
  norms->Update();

  mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);

  // Modify the shader to color based on model normal
  // To do this we have to modify the vertex shader
  // to pass the normal in model coordinates
  // through to the fragment shader. By default the normal
  // is converted to View coordinates and then passed on.
  // We keep that, but add a varying for the original normal.
  // Then we modify the fragment shader to set the diffuse color
  // based on that normal. First lets modify the vertex
  // shader
  mapper->AddShaderReplacement(
    vtkShader::Vertex,
    "//VTK::Normal::Dec", // replace the normal block
    true, // before the standard replacements
    "//VTK::Normal::Dec\n" // we still want the default
    "  varying vec3 myNormalMCVSOutput;\n", //but we add this
    false // only do it once
    );
  mapper->AddShaderReplacement(
    vtkShader::Vertex,
    "//VTK::Normal::Impl", // replace the normal block
    true, // before the standard replacements
    "//VTK::Normal::Impl\n" // we still want the default
    "  myNormalMCVSOutput = normalMC;\n", //but we add this
    false // only do it once
    );

  // now modify the fragment shader
  mapper->AddShaderReplacement(
    vtkShader::Fragment,  // in the fragment shader
    "//VTK::Normal::Dec", // replace the normal block
    true, // before the standard replacements
    "//VTK::Normal::Dec\n" // we still want the default
    "  varying vec3 myNormalMCVSOutput;\n", //but we add this
    false // only do it once
    );
  mapper->AddShaderReplacement(
    vtkShader::Fragment,  // in the fragment shader
    "//VTK::Normal::Impl", // replace the normal block
    true, // before the standard replacements
    "//VTK::Normal::Impl\n" // we still want the default calc
    "  diffuseColor = abs(myNormalMCVSOutput);\n", //but we add this
    false // only do it once
    );

  renderWindow->Render();
  renderer->GetActiveCamera()->SetPosition(-0.2,0.4,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
