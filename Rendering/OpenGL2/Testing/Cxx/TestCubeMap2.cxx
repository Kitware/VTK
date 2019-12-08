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
#include "vtkPLYReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkSkybox.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include "vtkLight.h"

//----------------------------------------------------------------------------
int TestCubeMap2(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 7.0, 1.0);
  renderer->AddLight(light);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  vtkNew<vtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  const char* fpath[] = { "Data/skybox/posx.jpg", "Data/skybox/negx.jpg", "Data/skybox/posy.jpg",
    "Data/skybox/negy.jpg", "Data/skybox/posz.jpg", "Data/skybox/negz.jpg" };

  vtkNew<vtkTexture> texture;
  texture->CubeMapOn();
  texture->InterpolateOn();
  texture->RepeatOff();
  texture->EdgeClampOn();

  // mipmapping works on many systems but is not
  // core 3.2 for cube maps. VTK will silently
  // ignore it if it is not supported. We commented it
  // out here to make valid images easier
  // texture->MipmapOn();

  for (int i = 0; i < 6; i++)
  {
    const char* fName = vtkTestUtilities::ExpandDataFileName(argc, argv, fpath[i]);
    vtkNew<vtkJPEGReader> imgReader;
    imgReader->SetFileName(fName);
    vtkNew<vtkImageFlip> flip;
    flip->SetInputConnection(imgReader->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    texture->SetInputConnection(i, flip->GetOutputPort(0));
    delete[] fName;
  }

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetPosition(0, 0, 0);
  actor->SetScale(6.0, 6.0, 6.0);
  actor->GetProperty()->SetSpecular(0.8);
  actor->GetProperty()->SetSpecularPower(20);
  actor->GetProperty()->SetDiffuse(0.1);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.0, 0.4);
  actor->GetProperty()->SetAmbientColor(0.4, 0.0, 1.0);
  renderer->AddActor(actor);
  actor->SetTexture(texture);
  actor->SetMapper(mapper);

  vtkShaderProperty* sp = actor->GetShaderProperty();
  sp->AddVertexShaderReplacement("//VTK::PositionVC::Dec", // replace
    true,                                                  // before the standard replacements
    "//VTK::PositionVC::Dec\n"                             // we still want the default
    "out vec3 TexCoords;\n",
    false // only do it once
  );
  sp->AddVertexShaderReplacement("//VTK::PositionVC::Impl", // replace
    true,                                                   // before the standard replacements
    "//VTK::PositionVC::Impl\n"                             // we still want the default
    "vec3 camPos = -MCVCMatrix[3].xyz * mat3(MCVCMatrix);\n"
    "TexCoords.xyz = reflect(vertexMC.xyz - camPos, normalize(normalMC));\n",
    false // only do it once
  );
  sp->AddFragmentShaderReplacement("//VTK::Light::Dec", // replace
    true,                                               // before the standard replacements
    "//VTK::Light::Dec\n"                               // we still want the default
    "in vec3 TexCoords;\n",
    false // only do it once
  );
  sp->AddFragmentShaderReplacement("//VTK::Light::Impl", // replace
    true,                                                // before the standard replacements
    "  vec3 cubeColor = texture(actortexture, normalize(TexCoords)).xyz;\n"
    "//VTK::Light::Impl\n"
    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular + specularColor*cubeColor, "
    "opacity);\n", // we still want the default
    false          // only do it once
  );

  vtkNew<vtkSkybox> world;
  world->SetTexture(texture);
  renderer->AddActor(world);

  renderer->GetActiveCamera()->SetPosition(0.0, 0.55, 2.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.55, 0.0);
  renderer->GetActiveCamera()->SetViewAngle(60.0);
  renderer->GetActiveCamera()->Zoom(1.1);
  renderer->GetActiveCamera()->Azimuth(0);
  renderer->GetActiveCamera()->Elevation(5);
  renderer->GetActiveCamera()->Roll(-10);
  renderer->ResetCameraClippingRange();

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
