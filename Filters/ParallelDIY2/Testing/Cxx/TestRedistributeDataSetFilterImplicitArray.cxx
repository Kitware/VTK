#include <vtkDataSet.h>
#include <vtkImageData.h>
#include <vtkImplicitArray.h>
#include <vtkMPIController.h>
#include <vtkPointData.h>
#include <vtkRedistributeDataSetFilter.h>

#include <cstdlib>

namespace
{
bool RedistributeImageData(vtkMultiProcessController* controller);
}

int TestRedistributeDataSetFilterImplicitArray(int argc, char* argv[])
{
  int res = EXIT_SUCCESS;
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);
  if (!::RedistributeImageData(controller))
  {
    std::cout << "Could not redistribute image data" << std::endl;
    res = EXIT_FAILURE;
  }
  controller->Finalize();
  return res;
}

namespace
{

/*
 * This backend needs to be trivially cosntructible to be compatible with
 * `vtkImplicitArray<...>::NewInstance`. If it is not, then the new instance
 * returns an array without an instanced backend which leads to a segfault on
 * the first get. This is something that might need to be dealt with at some
 * point.
 */
struct Backend42
{
  double operator()(int) const { return 42.0; }
};

bool RedistributeImageData(vtkMultiProcessController* controller)
{
  int nPix = 100;
  int halfCells = nPix / 2;
  vtkNew<vtkImageData> baseGrid;
  vtkNew<vtkImplicitArray<Backend42>> vortex;
  vortex->SetName("42");
  baseGrid->GetPointData()->AddArray(vortex);
  baseGrid->GetPointData()->SetActiveVectors("42");
  if (controller->GetLocalProcessId() == 0)
  {
    baseGrid->SetExtent(-halfCells / 2, halfCells, -halfCells, halfCells, -halfCells, halfCells);
    baseGrid->SetSpacing(1.0 / nPix, 1.0 / nPix, 1.0 / nPix);
    vortex->SetNumberOfComponents(3);
    vortex->SetNumberOfTuples(baseGrid->GetNumberOfPoints());
  }
  else
  {
    baseGrid->SetExtent(-halfCells, -halfCells / 2, -halfCells, halfCells, -halfCells, halfCells);
    baseGrid->SetSpacing(1.0 / nPix, 1.0 / nPix, 1.0 / nPix);
    vortex->SetNumberOfComponents(3);
    vortex->SetNumberOfTuples(baseGrid->GetNumberOfPoints());
  }

  vtkNew<vtkRedistributeDataSetFilter> redistribute;
  redistribute->SetInputData(baseGrid);
  redistribute->Update();

  vtkDataSet* redDS = vtkDataSet::SafeDownCast(redistribute->GetOutput(0));

  int nPtsArr = redDS->GetPointData()->GetArray("42")->GetNumberOfTuples();
  int nPtsDS = redDS->GetNumberOfPoints();

  return (nPtsDS == nPtsArr) && (redDS->GetPointData()->GetArray("42")->GetComponent(0, 0) == 42);
}
} // namespace
