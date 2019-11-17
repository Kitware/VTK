/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"
#include "vtkTriangleMeshPointNormals.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestUserShader(int argc, char* argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  vtkNew<vtkTriangleMeshPointNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());
  norms->Update();

  mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper);
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
  vtkShaderProperty* sp = actor->GetShaderProperty();
  sp->AddVertexShaderReplacement("//VTK::Normal::Dec", // replace the normal block
    true,                                              // before the standard replacements
    "//VTK::Normal::Dec\n"                             // we still want the default
    "  out vec3 myNormalMCVSOutput;\n",                // but we add this
    false                                              // only do it once
  );
  sp->AddVertexShaderReplacement("//VTK::Normal::Impl", // replace the normal block
    true,                                               // before the standard replacements
    "//VTK::Normal::Impl\n"                             // we still want the default
    "  myNormalMCVSOutput = normalMC;\n",               // but we add this
    false                                               // only do it once
  );
  sp->AddVertexShaderReplacement("//VTK::Color::Impl", // dummy replacement for testing clear method
    true, "VTK::Color::Impl\n", false);
  sp->ClearVertexShaderReplacement("//VTK::Color::Impl", true);

  // now modify the fragment shader
  sp->AddFragmentShaderReplacement("//VTK::Normal::Dec", // replace the normal block
    true,                                                // before the standard replacements
    "//VTK::Normal::Dec\n"                               // we still want the default
    "  in vec3 myNormalMCVSOutput;\n",                   // but we add this
    false                                                // only do it once
  );
  sp->AddFragmentShaderReplacement("//VTK::Normal::Impl", // replace the normal block
    true,                                                 // before the standard replacements
    "//VTK::Normal::Impl\n"                               // we still want the default calc
    "  diffuseColor = abs(myNormalMCVSOutput);\n",        // but we add this
    false                                                 // only do it once
  );

  // Test enumerating shader replacements
  int nbReplacements = sp->GetNumberOfShaderReplacements();
  if (nbReplacements != 4)
  {
    return EXIT_FAILURE;
  }
  if (sp->GetNthShaderReplacementTypeAsString(0) != std::string("Vertex") ||
    sp->GetNthShaderReplacementTypeAsString(1) != std::string("Fragment") ||
    sp->GetNthShaderReplacementTypeAsString(2) != std::string("Vertex") ||
    sp->GetNthShaderReplacementTypeAsString(3) != std::string("Fragment"))
  {
    return EXIT_FAILURE;
  }

  renderWindow->Render();
  renderer->GetActiveCamera()->SetPosition(-0.2, 0.4, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
