
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestGradientBackground(int argc, char* argv[])
{
  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderer, ren);
  VTK_CREATE(vtkConeSource, cone);
  VTK_CREATE(vtkPolyDataMapper, map);
  VTK_CREATE(vtkActor, act);

  map->SetInputConnection(cone->GetOutputPort());
  act->SetMapper(map);
  ren->AddActor(act);
  ren->GradientBackgroundOn();
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

