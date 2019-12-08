
#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkCullerCollection.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMath.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTimerLog.h"

int TestManyActors(int argc, char* argv[])
{
  int numActors = 512;
  int numRenders = 100;
  bool interact = false;
  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-I"))
    {
      interact = true;
      continue;
    }
    if (!strcmp(argv[i], "-T") || !strcmp(argv[i], "-V") || !strcmp(argv[i], "-D"))
    {
      ++i;
      continue;
    }
    if (!strcmp(argv[i], "-N"))
    {
      ++i;
      numActors = atoi(argv[i]);
      continue;
    }
    if (!strcmp(argv[i], "-R"))
    {
      ++i;
      numRenders = atoi(argv[i]);
      continue;
    }
    cerr << argv[0] << " options:" << endl;
    cerr << " -N: Number of actors" << endl;
  }
  vtkSmartPointer<vtkSphereSource> source = vtkSmartPointer<vtkSphereSource>::New();
  source->Update();
  vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
  long side1 = std::lround(pow(static_cast<double>(numActors), 1.0 / 3.0));
  long side2 = std::lround(sqrt(numActors / static_cast<double>(side1)));
  long side3 = static_cast<long>(ceil(static_cast<double>(numActors) / side1 / side2));
  int actorId = 0;
  for (long i = 0; i < side1; ++i)
  {
    for (long j = 0; j < side2; ++j)
    {
      for (long k = 0; k < side3; ++k)
      {
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->StaticOn();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        mapper->SetInputConnection(source->GetOutputPort());
        mapper->StaticOn();
        actor->SetMapper(mapper);
        actor->SetPosition(i, j, k);
        ren->AddActor(actor);
        ++actorId;
        if (actorId >= numActors)
        {
          break;
        }
      }
      if (actorId >= numActors)
      {
        break;
      }
    }
    if (actorId >= numActors)
    {
      break;
    }
  }
  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  ren->ResetCamera();
  ren->RemoveCuller(ren->GetCullers()->GetLastItem());
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  iren->SetInteractorStyle(style);

  cerr << "number of actors: " << numActors << endl;
  cerr << "number of renders: " << numRenders << endl;

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
  iren->Initialize();
  iren->SetEventPosition(100, 100);
  iren->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  for (int i = 0; i < numRenders; ++i)
  {
    iren->SetEventPosition(100, 100 + i);
    iren->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
  }
  iren->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  cerr << "render time per actor: " << elapsed / numRenders / numActors << endl;

  if (interact)
  {
    iren->Start();
  }

  return 0;
}
