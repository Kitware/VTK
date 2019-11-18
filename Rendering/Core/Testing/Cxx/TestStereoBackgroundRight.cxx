#include "vtkConeSource.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestStereoBackgroundRight(int argc, char* argv[])
{
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderer, ren);
  VTK_CREATE(vtkConeSource, cone);
  VTK_CREATE(vtkPolyDataMapper, map);
  VTK_CREATE(vtkActor, act);
  VTK_CREATE(vtkTexture, textr);
  VTK_CREATE(vtkJPEGReader, imgReader);
  VTK_CREATE(vtkImageData, image);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");
  imgReader->SetFileName(fname);
  delete[] fname;

  imgReader->Update();
  textr->SetInputConnection(imgReader->GetOutputPort(0));

  map->SetInputConnection(cone->GetOutputPort(0));
  act->SetMapper(map);
  act->GetProperty()->BackfaceCullingOn();
  ren->AddActor(act);
  ren->TexturedBackgroundOn();
  ren->SetRightBackgroundTexture(textr);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  win->SetMultiSamples(0);
  win->SetStereoTypeToRight();
  win->SetStereoRender(true);
  win->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
