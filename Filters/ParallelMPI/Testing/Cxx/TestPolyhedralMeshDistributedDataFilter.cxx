#include "vtkDistributedDataFilter.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <string>

void AbortTest(vtkMPIController* controller, const std::string& message)
{
  if (controller->GetLocalProcessId() == 0)
  {
    vtkErrorWithObjectMacro(nullptr, << message.c_str());
  }
  controller->Finalize();
  exit(EXIT_FAILURE);
}

int TestPolyhedralMeshDistributedDataFilter(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);

  const int rank = controller->GetLocalProcessId();

  vtkMultiProcessController::SetGlobalController(controller);

  vtkSmartPointer<vtkUnstructuredGrid> ug;
  // Load a full polyhedral mesh on rank 0, others have an empty piece
  if (rank == 0)
  {
    vtkNew<vtkXMLUnstructuredGridReader> r;
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/voronoiMesh.vtu");
    r->SetFileName(fname);
    r->Update();
    ug = r->GetOutput();
    delete[] fname;
  }
  else
  {
    ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
  }

  // Fetch number of cells of the full distributed mesh
  vtkIdType allNbCells = 0;
  vtkIdType localNbCells = ug->GetNumberOfCells();
  controller->AllReduce(&localNbCells, &allNbCells, 1, vtkCommunicator::SUM_OP);
  if (allNbCells == 0)
  {
    AbortTest(controller, "ERROR: Check grid failed!");
  }

  // Distribute the mesh with the D3 filter
  vtkNew<vtkDistributedDataFilter> d3;
  d3->SetInputData(ug);
  d3->SetController(controller);
  d3->SetBoundaryMode(0);
  d3->SetUseMinimalMemory(0);
  d3->SetMinimumGhostLevel(0);
  d3->Update();

  ug = vtkUnstructuredGrid::SafeDownCast(d3->GetOutput());

  // Check that each rank own a piece of the full mesh
  int success = ug->GetNumberOfCells() != 0 ? 1 : 0;
  int allsuccess = 0;
  controller->AllReduce(&success, &allsuccess, 1, vtkCommunicator::MIN_OP);
  if (allsuccess == 0)
  {
    AbortTest(controller, "ERROR: Invalid mesh distribution - some ranks have 0 cell.");
  }

  // Check that input and output distributed meshes have same number of cells
  vtkIdType allOutNbCells = 0;
  vtkIdType localOutNbCells = ug->GetNumberOfCells();
  controller->AllReduce(&localOutNbCells, &allOutNbCells, 1, vtkCommunicator::SUM_OP);
  if (allNbCells != allOutNbCells)
  {
    AbortTest(controller,
      "ERROR: Invalid mesh distribution - input and output mesh have different number of cells.");
  }

  controller->Finalize();

  return EXIT_SUCCESS;
}
