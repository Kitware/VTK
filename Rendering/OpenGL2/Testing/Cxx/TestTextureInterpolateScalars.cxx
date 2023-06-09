#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkElevationFilter.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTexturedSphereSource.h"

//------------------------------------------------------------------------------
int TestTextureInterpolateScalars(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.5, 0.5);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkTexturedSphereSource> sphere;
  sphere->SetThetaResolution(64);
  sphere->SetPhiResolution(32);

  vtkNew<vtkElevationFilter> elevationFilter;
  elevationFilter->SetLowPoint(0, 0, -0.5);
  elevationFilter->SetHighPoint(0, 0, 0.5);
  elevationFilter->SetInputConnection(sphere->GetOutputPort());

  // Use Paraview's Rainbow Desaturated colormap preset
  vtkNew<vtkDiscretizableColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.11, 0.278431, 0.278431, 0.858824);
  ctf->AddRGBPoint(0.22, 0, 0, 0.360784);
  ctf->AddRGBPoint(0.33, 0, 1, 1);
  ctf->AddRGBPoint(0.44, 0, 0.501961, 0);
  ctf->AddRGBPoint(0.55, 1, 1, 0);
  ctf->AddRGBPoint(0.66, 1, 0.380392, 0);
  ctf->AddRGBPoint(0.77, 0.419608, 0, 0);
  ctf->AddRGBPoint(0.88, 0.878431, 0.301961, 0.301961);
  ctf->DiscretizeOn();
  ctf->SetNumberOfValues(8);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elevationFilter->GetOutputPort());
  mapper->SetLookupTable(ctf);
  mapper->SetColorModeToMapScalars();
  mapper->InterpolateScalarsBeforeMappingOn();

  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  const char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/clouds.jpeg");
  vtkNew<vtkJPEGReader> reader;
  reader->SetFileName(file);
  delete[] file;

  vtkNew<vtkTexture> tex;
  tex->InterpolateOn();
  tex->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE);
  tex->SetInputConnection(reader->GetOutputPort());
  actor->SetTexture(tex);

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(80);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
