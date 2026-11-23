#include "vtkConduitSource.h"
#include "vtkDataObjectToConduit.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

//----------------------------------------------------------------------------
void TestFailingNode()
{
  vtkNew<vtkImageData> image;
  conduit_cpp::Node mesh;
  vtkDataObjectToConduit::FillConduitNode(image, mesh);
  // We make uncorrect data on purpose by only setting coordset on process 0 and not process 1.
  if (vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() == 1)
  {
    mesh["topologies/mesh"].remove("coordset");
  }

  vtkNew<vtkConduitSource> source;
  source->SetNode(conduit_cpp::c_node(&mesh));
  source->Update();
}

//----------------------------------------------------------------------------
bool TestMismatchedTables()
{
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller->GetNumberOfProcesses() < 2)
  {
    return true;
  }
  conduit_cpp::Node tables;
  if (controller->GetLocalProcessId() == 0)
  {
    tables["probes/values/time"].set_float64_vector({ 0.0, 1.0 });
    tables["forces/values/drag"].set_float64_vector({ 0.3 });
  }

  vtkNew<vtkConduitSource> source;
  source->SetUseTableProtocol(true);
  source->SetNode(conduit_cpp::c_node(&tables));
  return source->GetExecutive()->Update() == 0;
}

//----------------------------------------------------------------------------
int TestConduitSourceOneFailingNode(int argc, char** argv)
{

  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  // This test is a smoke test to make sure vtkConduitSource filter does not hang in MPI mode.
  // Therefore this test only returns success but would fail in timeout if the conduit was hanging.
  // As we expect error messages, we disable it for this test
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);
  TestFailingNode();
  const bool mismatchFails = TestMismatchedTables();

  controller->Finalize();

  return mismatchFails ? EXIT_SUCCESS : EXIT_FAILURE;
}
