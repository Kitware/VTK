#include "vtkDIYDataExchanger.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#include <map>
#include <vector>

static vtkSmartPointer<vtkDataSet> GetDataSet(int sourceId)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();

  auto data = sphere->GetOutput();
  vtkNew<vtkIntArray> array;
  array->SetName("SourceRank");
  array->SetNumberOfTuples(1);
  array->SetTypedComponent(0, 0, sourceId);
  data->GetFieldData()->AddArray(array);
  return data;
}

static bool DoTest(
  vtkMultiProcessController* controller, const std::map<int, std::vector<int>>& communication)
{
  const int nranks = controller->GetNumberOfProcesses();
  const int rank = controller->GetLocalProcessId();

  std::vector<vtkSmartPointer<vtkDataSet>> sendBuffer;
  std::vector<int> sendCounts(nranks);

  auto iter = communication.find(rank);
  if (iter != communication.end())
  {
    auto& send_vector = (iter->second);
    for (int cc = 0; cc < nranks && cc < static_cast<int>(send_vector.size()); ++cc)
    {
      for (int count = 0; count < send_vector[cc]; ++count)
      {
        sendBuffer.push_back(GetDataSet(rank));
        ++sendCounts[cc];
      }
    }
  }

  std::vector<vtkSmartPointer<vtkDataSet>> recvBuffer;
  std::vector<int> recvCounts;
  vtkNew<vtkDIYDataExchanger> exchanger;
  exchanger->SetController(controller);
  exchanger->AllToAll(sendBuffer, sendCounts, recvBuffer, recvCounts);

  return true;
}

int TestDIYDataExchanger(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  if (controller->GetNumberOfProcesses() != 3)
  {
    vtkLogF(ERROR, "This test expects exactly 3 ranks.");
    return EXIT_FAILURE;
  }

  // Each rank sends 1 dataset to each.
  vtkLogF(INFO, "send 1 dataset to each rank");
  DoTest(controller, { { 0, { 1, 1, 1 } }, { 1, { 1, 1, 1 } }, { 2, { 1, 1, 1 } } });

  // Only one rank sends data to all.
  vtkLogF(INFO, "only rank=2 sends data to each rank");
  DoTest(controller, { { 0, {} }, { 1, { 1, 1, 1 } }, { 2, {} } });

  // no one sends anything.
  vtkLogF(INFO, "no rank sends any data");
  DoTest(controller, {});

  controller->Finalize();
  return EXIT_SUCCESS;
}
