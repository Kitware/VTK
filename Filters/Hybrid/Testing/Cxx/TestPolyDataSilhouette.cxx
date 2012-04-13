#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataSilhouette.h>
#include <vtkSphereSource.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkCleanPolyData.h>
#include <vtkTestUtilities.h>

int TestPolyDataSilhouette(int argc, char *argv[])
{
  vtkSmartPointer<vtkPolyData> polyData;
  char* fname(NULL);
  if (argc < 2)
    {
    vtkSmartPointer<vtkSphereSource> sphereSource =
      vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->Update();

    polyData = sphereSource->GetOutput();
    }
  else
    {
    fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
    vtkSmartPointer<vtkXMLPolyDataReader> reader =
      vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader->SetFileName(fname);

    vtkSmartPointer<vtkCleanPolyData> clean =
      vtkSmartPointer<vtkCleanPolyData>::New();
    clean->SetInputConnection(reader->GetOutputPort());
    clean->Update();

    polyData = clean->GetOutput();
    }

  //create mapper and actor for original model
  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(polyData);

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToFlat();

  //create renderer and renderWindow
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  renderer->AddActor(actor); //view the original model

  //Compute the silhouette
  vtkSmartPointer<vtkPolyDataSilhouette> silhouette =
    vtkSmartPointer<vtkPolyDataSilhouette>::New();
  silhouette->SetInputData(polyData);
  silhouette->SetCamera(renderer->GetActiveCamera());
  silhouette->SetEnableFeatureAngle(0);

  //create mapper and actor for silouette
  vtkSmartPointer<vtkPolyDataMapper> mapper2 =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper2->SetInputConnection(silhouette->GetOutputPort());

  vtkSmartPointer<vtkActor> actor2 =
    vtkSmartPointer<vtkActor>::New();
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetColor(1.0, 0.3882, 0.2784); // tomato
  actor2->GetProperty()->SetLineWidth(5);
  renderer->AddActor(actor2);
  renderer->SetBackground(.1, .2, .3);
  renderer->ResetCamera();

  //you MUST NOT call renderWindow->Render() before
  //iren->SetRenderWindow(renderWindow);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renderWindow);

  //render and interact
  renderWindow->Render();
  iren->Start();

  delete[] fname;
  return EXIT_SUCCESS;
}
