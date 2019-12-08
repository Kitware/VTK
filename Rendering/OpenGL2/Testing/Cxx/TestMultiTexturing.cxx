#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTIFFReader.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTexturedSphereSource.h"

//----------------------------------------------------------------------------
int TestMultiTexturing(int argc, char* argv[])
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
  sphere->Update();
  vtkPolyData* pd = sphere->GetOutput();

  vtkFloatArray* tcoord = vtkFloatArray::SafeDownCast(pd->GetPointData()->GetTCoords());
  vtkNew<vtkFloatArray> tcoord2;
  tcoord2->SetNumberOfComponents(2);
  tcoord2->SetNumberOfTuples(tcoord->GetNumberOfTuples());
  for (int i = 0; i < tcoord->GetNumberOfTuples(); ++i)
  {
    float tmp[2];
    tcoord->GetTypedTuple(i, tmp);
    // mess with the tcoords to make sure
    // this array is getting used
    tcoord2->SetTuple2(i, tmp[0], tmp[1] * 2.0);
  }
  tcoord2->SetName("tcoord2");
  pd->GetPointData()->AddArray(tcoord2);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(pd);
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  const char* file1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/raster.tif");
  vtkNew<vtkTIFFReader> reader1;
  reader1->SetFileName(file1);
  delete[] file1;

  vtkNew<vtkTexture> tex1;
  tex1->InterpolateOn();
  tex1->SetInputConnection(reader1->GetOutputPort());
  actor->GetProperty()->SetTexture("earth_color", tex1);

  const char* file2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/clouds.jpeg");
  vtkNew<vtkJPEGReader> reader2;
  reader2->SetFileName(file2);
  delete[] file2;

  vtkNew<vtkTexture> tex2;
  tex2->InterpolateOn();
  tex2->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE);
  tex2->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
  tex2->SetInputConnection(reader2->GetOutputPort());
  actor->GetProperty()->SetTexture("skyclouds", tex2);

  mapper->MapDataArrayToMultiTextureAttribute(
    "skyclouds", "tcoord2", vtkDataObject::FIELD_ASSOCIATION_POINTS);

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(-45);
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
