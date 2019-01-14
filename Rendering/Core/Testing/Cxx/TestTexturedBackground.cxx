#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkImageData.h"
#include "vtkPolyDataMapper.h"
#include "vtkJPEGReader.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkRegressionTestImage.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestTexturedBackground(int argc, char* argv[])
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

  char* fname = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/beach.jpg");
  imgReader->SetFileName(fname);
  delete [] fname;

  imgReader->Update();

  textr->SetInputConnection(imgReader->GetOutputPort(0));

  // Do not call this.
//  textr->Update();

  map->SetInputConnection(cone->GetOutputPort(0));
  act->SetMapper(map);
  ren->AddActor(act);
  ren->TexturedBackgroundOn();
//  ren->GradientBackgroundOn();
  ren->SetBackgroundTexture(textr);
  ren->SetBackground(0.8, 0.4, 0.1);
  ren->SetBackground2(0.1, 0.4, 0.8);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  win->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
