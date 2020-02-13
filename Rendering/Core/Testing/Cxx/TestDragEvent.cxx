#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

class vtkTestDragInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkTestDragInteractorStyle* New();
  vtkTypeMacro(vtkTestDragInteractorStyle, vtkInteractorStyleTrackballCamera);

  void OnDropLocation(double* position) override
  {
    this->Location[0] = position[0];
    this->Location[1] = position[1];
    this->Location[2] = 0.0;
  }

  void OnDropFiles(vtkStringArray* filePaths) override
  {
    vtkRenderWindowInteractor* rwi = this->GetInteractor();

    const char* path = filePaths->GetValue(0);

    vtkNew<vtkXMLPolyDataReader> reader;
    reader->SetFileName(path);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkRenderer* ren = rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    ren->AddActor(actor);

    // move actor to location
    ren->SetDisplayPoint(this->Location);
    ren->DisplayToWorld();
    actor->SetPosition(ren->GetWorldPoint());

    rwi->GetRenderWindow()->Render();
  }

  double Location[3];
};

vtkStandardNewMacro(vtkTestDragInteractorStyle);

int TestDragEvent(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkTestDragInteractorStyle> style;
  iren->SetInteractorStyle(style);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(5.0);
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  renderer->AddActor(actor);

  renWin->Render();

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");

  vtkNew<vtkStringArray> pathArray;
  pathArray->InsertNextValue(fname);

  delete[] fname;

  // Manually invoke drag and drop events for this test
  // These events are usually invoked when a file is dropped on the render window
  // from a file manager
  double loc[2] = { 100.0, 250.0 };
  iren->InvokeEvent(vtkCommand::UpdateDropLocationEvent, loc);
  iren->InvokeEvent(vtkCommand::DropFilesEvent, pathArray);

  renWin->Render();

  // Compare image
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
