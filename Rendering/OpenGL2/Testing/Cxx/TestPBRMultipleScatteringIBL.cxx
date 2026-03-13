#include "vtkActor.h"
#include "vtkHDRReader.h"
#include "vtkOpenGLSkybox.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTexture.h"

int TestPBRMultipleScatteringIBL(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> interactor;
  renderWindow->SetInteractor(interactor);

  vtkNew<vtkOpenGLSkybox> skybox;
  vtkNew<vtkHDRReader> hdrReader;
  vtkNew<vtkTesting> testing;
  std::string hdrFile =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/spiaggia_di_mondello_1k.hdr");
  hdrReader->SetFileName(hdrFile.c_str());
  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->MipmapOn();
  texture->InterpolateOn();
  texture->SetInputConnection(hdrReader->GetOutputPort());

  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(texture);

  skybox->SetFloorRight(0.0, 0.0, 1.0);
  skybox->SetProjectionToSphere();
  skybox->SetTexture(texture);
  renderer->AddActor(skybox);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(75);
  sphere->SetPhiResolution(75);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  for (int i = 0; i < 6; ++i)
  {
    vtkNew<vtkActor> actor;
    actor->SetPosition(i, 0.0, 0.0);
    actor->SetMapper(mapper);
    vtkProperty* prop = actor->GetProperty();
    prop->SetInterpolationToPBR();
    prop->SetMetallic(1.0);
    prop->SetRoughness(i / 5.0);
    renderer->AddActor(actor);
  }
  renderer->ResetCamera();

  renderWindow->Render();
  interactor->Start();

  return EXIT_SUCCESS;
}
