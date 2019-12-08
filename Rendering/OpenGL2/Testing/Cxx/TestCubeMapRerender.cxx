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
#include "vtkImageFlip.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

//----------------------------------------------------------------------------
int TestCubeMapRerender(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  vtkNew<vtkTexture> texture;
  texture->CubeMapOn();

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  vtkNew<vtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  const char* fpath[] = { "Data/skybox-px.jpg", "Data/skybox-nx.jpg", "Data/skybox-py.jpg",
    "Data/skybox-ny.jpg", "Data/skybox-pz.jpg", "Data/skybox-nz.jpg" };

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkJPEGReader> imgReader;
    const char* fName = vtkTestUtilities::ExpandDataFileName(argc, argv, fpath[i]);
    imgReader->SetFileName(fName);
    delete[] fName;
    vtkNew<vtkImageFlip> flip;
    flip->SetInputConnection(imgReader->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    texture->SetInputConnection(i, flip->GetOutputPort(0));
  }

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetTexture(texture);
  actor->SetMapper(mapper);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.4);
  renderWindow->Render();

  vtkShaderProperty* sp = actor->GetShaderProperty();

  // Add new code in default VTK vertex shader
  sp->AddVertexShaderReplacement("//VTK::PositionVC::Dec", // replace the normal block
    true,                                                  // before the standard replacements
    "//VTK::PositionVC::Dec\n"                             // we still want the default
    "out vec3 TexCoords;\n",
    false // only do it once
  );
  sp->AddVertexShaderReplacement("//VTK::PositionVC::Impl", // replace the normal block
    true,                                                   // before the standard replacements
    "//VTK::PositionVC::Impl\n"                             // we still want the default
    "vec3 camPos = -MCVCMatrix[3].xyz * mat3(MCVCMatrix);\n"
    "TexCoords.xyz = reflect(vertexMC.xyz - camPos, normalize(normalMC));\n",
    false // only do it once
  );

  // Replace VTK fragment shader
  sp->SetFragmentShaderCode("//VTK::System::Dec\n" // always start with this line
                            "//VTK::Output::Dec\n" // always have this line in your FS
                            "in vec3 TexCoords;\n"
                            "uniform samplerCube texture_0;\n"
                            "void main () {\n"
                            "  gl_FragData[0] = texture(texture_0, TexCoords);\n"
                            "}\n");

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.4);
  renderWindow->Render();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindow->GetInteractor()->SetInteractorStyle(style);

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
