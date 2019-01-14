#include "vtkImageData.h"
#include "vtkImageReader2.h"
#include "vtkImageToImageStencil.h"
#include "vtkImageToPoints.h"
#include "vtkImageMapToColors.h"
#include "vtkScalarsToColors.h"
#include "vtkGlyph3DMapper.h"
#include "vtkSphereSource.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkGlyph3DMapper.h"
#include "vtkTestUtilities.h"
#include "vtkSmartPointer.h"

int TestImageToPoints(int argc, char *argv[])
{
  // Test vtkImageToPoints by converting a few slices of the headsq
  // data set into points and glyphing them.

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/headsq/quarter");
  std::string filename = fname;
  delete [] fname;

  int extent[6] = { 0, 63, 0, 63, 0, 3 };
  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 3.2, 3.2, 1.5 };
  double center[3] = { 0.5*3.2*63, 0.5*3.2*63, 0.5*1.5*3 };

  vtkSmartPointer<vtkImageReader2> reader =
    vtkSmartPointer<vtkImageReader2>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(extent);
  reader->SetDataOrigin(origin);
  reader->SetDataSpacing(spacing);
  reader->SetFileNameSliceOffset(40);
  reader->SetFilePrefix(filename.c_str());

  // convert the image into color scalars
  vtkSmartPointer<vtkScalarsToColors> table =
    vtkSmartPointer<vtkScalarsToColors>::New();
  table->SetRange(0, 2000);

  vtkSmartPointer<vtkImageMapToColors> colors =
    vtkSmartPointer<vtkImageMapToColors>::New();
  colors->SetInputConnection(reader->GetOutputPort());
  colors->SetLookupTable(table);
  colors->SetOutputFormatToRGB();

  // generate a stencil by thresholding the image
  vtkSmartPointer<vtkImageToImageStencil> stencil =
    vtkSmartPointer<vtkImageToImageStencil>::New();
  stencil->SetInputConnection(reader->GetOutputPort());
  stencil->ThresholdBetween(800, 4000);

  // generate a point set
  vtkSmartPointer<vtkImageToPoints> imageToPointSet =
    vtkSmartPointer<vtkImageToPoints>::New();
  imageToPointSet->SetInputConnection(colors->GetOutputPort());
  imageToPointSet->SetStencilConnection(stencil->GetOutputPort());
  imageToPointSet->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
  imageToPointSet->Update();

  // generate a sphere for each point
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetRadius(1.5);

  // display the points as glyphs
  vtkSmartPointer<vtkGlyph3DMapper> mapper =
    vtkSmartPointer<vtkGlyph3DMapper>::New();
  mapper->ScalingOff();
  mapper->SetInputConnection(imageToPointSet->GetOutputPort());
  mapper->SetSourceConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetSize(256, 256);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->AddViewProp(actor);
  renWin->AddRenderer(renderer);

  vtkCamera *camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(center);
  camera->SetPosition(center[0], center[1], center[2] - 400.0);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
