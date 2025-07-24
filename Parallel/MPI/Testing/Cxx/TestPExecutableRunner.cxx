// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPExecutableRunner.h"

#include <optional>
#include <string>

namespace
{

bool RunTestCase(vtkMPIController* controller, const std::string& command,
  std::optional<int> commandProcessId, const std::vector<std::string>& expectedResults)
{
  if (expectedResults.size() != (size_t)controller->GetNumberOfProcesses())
  {
    vtkLogF(ERROR,
      "ExpectedResults vector should have %i elements, but its number of elements is %i",
      controller->GetNumberOfProcesses(), (int)expectedResults.size());
    return false;
  }

  int localProcessId = controller->GetLocalProcessId();

  vtkNew<vtkPExecutableRunner> executableRunner;
  executableRunner->SetExecutionProcessId(commandProcessId.value_or(-1));
  executableRunner->SetCommand(command.c_str());
  executableRunner->Execute();

  int returnValue = executableRunner->GetReturnValue();
  if (returnValue > 0)
  {
    vtkLogF(ERROR, "Error when executing command: %s.", executableRunner->GetStdErr());
    return false;
  }

  std::string commandResult = executableRunner->GetStdOut();
  const std::string& expectedResult = expectedResults[static_cast<size_t>(localProcessId)];
  // Verify that the command result is the expected one
  if (commandResult != expectedResult)
  {
    vtkLogF(ERROR, "Expected %s command result but got %s.",
      expectedResult.empty() ? "[empty]" : expectedResult.c_str(), commandResult.c_str());
    return false;
  }

  return true;
}

}

int TestPExecutableRunner(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 1);
  controller->SetGlobalController(controller);

  std::string message = "Hello World!";
  std::string command;
#ifdef _WIN32
  command += "cmd.exe /c ";
#endif
  command += " echo " + message;

  int numberOfProcesses = controller->GetNumberOfProcesses();

  bool testResult = true;

  {
    // Test vtkPExecutableRunner when executing a command only on rank 0
    std::vector<std::string> testCaseExpectedResults;
    testCaseExpectedResults.resize(numberOfProcesses, "");
    testCaseExpectedResults[0] = message;
    testResult &= ::RunTestCase(controller, command, 0, testCaseExpectedResults);
  }

  // Test vtkPExecutableRunner when executing a command on all ranks
  {
    std::vector<std::string> testCaseExpectedResults;
    testCaseExpectedResults.resize(numberOfProcesses, message);
    testResult &= ::RunTestCase(controller, command, std::nullopt, testCaseExpectedResults);
  }

  controller->SetGlobalController(nullptr);
  controller->Finalize();

  return testResult ? EXIT_SUCCESS : EXIT_FAILURE;
}
