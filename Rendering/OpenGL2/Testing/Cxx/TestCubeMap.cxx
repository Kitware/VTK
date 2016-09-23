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
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTextureObject.h"

//----------------------------------------------------------------------------
int TestCubeMap(int argc, char *argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.Get());

  // We call Render to create the OpenGL context as it will
  // be needed by the texture object
  renderWindow->Render();

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);

  vtkNew<vtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  vtkSmartPointer<vtkImageData> imgs[6];
  const char* fpath[] =
    {
    "Data/skybox-px.jpg",
    "Data/skybox-nx.jpg",
    "Data/skybox-py.jpg",
    "Data/skybox-ny.jpg",
    "Data/skybox-pz.jpg",
    "Data/skybox-nz.jpg"
    };

  void* images[6];

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkJPEGReader> imgReader;
    imgReader->SetFileName(
      vtkTestUtilities::ExpandDataFileName(argc, argv, fpath[i]));
    vtkNew<vtkImageFlip> flip;
    flip->SetInputConnection(imgReader->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    flip->Update();
    imgs[i] = flip->GetOutput();
    images[i] = imgs[i]->GetScalarPointer();
  }

  int dims[3];
  imgs[0]->GetDimensions(dims);

  // Create a texture object from our set of cube map images
  vtkNew<vtkTextureObject> texObject;
  texObject->SetContext(
    vtkOpenGLRenderWindow::SafeDownCast(renderWindow.Get()));
  texObject->CreateCubeFromRaw(
    dims[0], dims[1], 3, imgs[0]->GetScalarType(), images);

  // Setup a texture with our home made texture object
  vtkNew<vtkOpenGLTexture> texture;
  texture->SetTextureObject(texObject.Get());

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  vtkNew<vtkActor> actor;
  renderer->AddActor(actor.Get());
  actor->SetTexture(texture.Get());
  actor->SetMapper(mapper.Get());

   // Add new code in default VTK vertex shader
  mapper->AddShaderReplacement(
    vtkShader::Vertex,
    "//VTK::PositionVC::Dec", // replace the normal block
    true, // before the standard replacements
    "//VTK::PositionVC::Dec\n" // we still want the default
    "varying vec3 TexCoords;\n",
    false // only do it once
    );
  mapper->AddShaderReplacement(
    vtkShader::Vertex,
    "//VTK::PositionVC::Impl", // replace the normal block
    true, // before the standard replacements
    "//VTK::PositionVC::Impl\n" // we still want the default
    "vec3 camPos = -MCVCMatrix[3].xyz * mat3(MCVCMatrix);\n"
    "TexCoords.xyz = reflect(vertexMC.xyz - camPos, normalize(normalMC));\n",
    false // only do it once
    );

  // Replace VTK fragment shader
  mapper->SetFragmentShaderCode(
    "//VTK::System::Dec\n"  // always start with this line
    "//VTK::Output::Dec\n"  // always have this line in your FS
    "varying vec3 TexCoords;\n"
    "uniform samplerCube CubeMap;\n"
    "void main () {\n"
    "  gl_FragData[0] = texture(CubeMap, TexCoords);\n"
    "}\n"
    );

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.4);
  renderWindow->Render();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindow->GetInteractor()->SetInteractorStyle(style.Get());

  int retVal = vtkRegressionTestImage(renderWindow.Get());
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
