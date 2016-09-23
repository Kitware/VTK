#include <vtkNew.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkPTSReader.h>

int TestPTSReader(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Required parameters: <filename> maxNumberOfPoints(optional)" << endl;
    return EXIT_FAILURE;
  }

  std::string inputFilename = argv[1];

  vtkNew<vtkPTSReader> reader;
  reader->SetFileName(inputFilename.c_str());
  reader->SetLimitToMaxNumberOfPoints(true);
  reader->SetMaxNumberOfPoints(100000);


  reader->Update();

  // Visualize
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  renderer->AddActor(actor.GetPointer());
  renderer->SetBackground(.3, .6, .3); // Background color green

  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
