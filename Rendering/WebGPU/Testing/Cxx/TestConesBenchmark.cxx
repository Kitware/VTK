#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkConeSource.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"

int TestConesBenchmark(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  double x = 0.0, y = 0.0, z = 0.0;
  double spacingX = 2.0, spacingY = 2.0, spacingZ = 2.0;
  for (int k = 0; k < 1; ++k)
  {
    for (int j = 0; j < 8; ++j)
    {
      for (int i = 0; i < 8; ++i)
      {
        vtkNew<vtkConeSource> cone;
        cone->SetCenter(x, y, z);
        x += spacingX;
        // map elevation output to graphics primitives.
        vtkNew<vtkPolyDataMapper> mapper;
        // mapper->SetLookupTable(lut);
        mapper->SetInputConnection(cone->GetOutputPort());
        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        // mapper->DebugOn();
        // actor->GetProperty()->SetPointSize(6.0);
        // actor->GetProperty()->SetRepresentationToPoints();
        mapper->Update();
        actor->SetOrigin(x, y, z);
        actor->RotateZ(i * j);
        renderer->AddActor(actor);
      }
      x = 0.0;
      y += spacingY;
    }
    y = 0.0;
    z += spacingZ;
  }

  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->Render();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);
  renWin->Render();

  iren->Start();
  return 0;
}
