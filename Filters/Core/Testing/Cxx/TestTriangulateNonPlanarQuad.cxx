#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTesting.h>
#include <vtkTriangleFilter.h>

#include <iostream>

int TestTriangulateNonPlanarQuad(int argc, char* argv[])
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();
  std::string inputFilename = dataRoot + "/Data/nonplanar_quad.vtk";

  // Read legacy VTK polydata
  vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
  reader->SetFileName(inputFilename.c_str());
  reader->Update();
  vtkSmartPointer<vtkPolyData> inPoly = reader->GetOutput();
  if (!inPoly || inPoly->GetNumberOfPoints() == 0)
  {
    std::cerr << "Failed to read polydata or polydata has no points.\n";
    return EXIT_FAILURE;
  }
  // Triangulate quad
  vtkSmartPointer<vtkTriangleFilter> triangulator = vtkSmartPointer<vtkTriangleFilter>::New();
  triangulator->SetInputData(inPoly);
  triangulator->Update();

  // Check that the output has two triangles
  int numTriangles = triangulator->GetOutput()->GetNumberOfCells();
  if (numTriangles != 2)
  {
    std::cerr << "Expected 2 triangles, but got " << numTriangles << std::endl;
    return EXIT_FAILURE;
  }

#if 0
  // Mapper & Actor
  vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(triangulator->GetOutputPort());
  mapper->ScalarVisibilityOff();
  vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Renderer / RenderWindow / Interactor
  vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
      vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  renWin->SetWindowName("Triangulated NonPlanar Quad - vtkTriangleFilter");
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
      vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  renderer->AddActor(actor);
  renderer->ResetCamera();
  renWin->Render();

  iren->Start();
#endif

  return EXIT_SUCCESS;
}
