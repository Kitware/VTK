// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogger.h"
#include "vtkMPIController.h"

namespace
{
bool TestDifferentBufferToEachProcess(vtkMultiProcessController* controller)
{
  int numberOfProcesses = controller->GetNumberOfProcesses();
  auto communicator = controller->GetCommunicator();

  int processId = communicator->GetLocalProcessId();

  std::vector<int> sendCounts(numberOfProcesses);
  std::vector<int> sendOffsets(numberOfProcesses);
  std::vector<int> recvCounts(numberOfProcesses);
  std::vector<int> recvOffsets(numberOfProcesses);

  int totalSend = 0;
  int totalRecv = 0;
  for (int i = 0; i < numberOfProcesses; ++i)
  {
    // We will send 1 value to processor 0, 2 values to processor 1 etc.
    sendCounts[i] = i + 1;
    sendOffsets[i] = totalSend;
    totalSend += sendCounts[i];

    // The first processor will receive 1 value from each processor, the second 2 values etc.
    recvCounts[i] = processId + 1;
    recvOffsets[i] = totalRecv;
    totalRecv += recvCounts[i];
  }

  std::vector<vtkIdType> sendBuffer(totalSend);

  // The send data buffer of the first processor will be filled with "0 1 1 2 2 2 3 3 3 3"
  // The send data buffer of the second processor will be filled with "10 11 11 12 12 12 13 13 13
  // 13" etc.
  for (int process = 0; process < numberOfProcesses; ++process)
  {
    for (int i = 0; i < sendCounts[process]; ++i)
    {
      sendBuffer[sendOffsets[process] + i] = 10 * processId + process;
    }
  }

  std::vector<vtkIdType> recvBuffer(totalRecv);
  std::vector<vtkIdType> recvBufferMPI(totalRecv);

  // Use our custom function of AllToAllVVoidArray
  bool result =
    communicator->vtkCommunicator::AllToAllVVoidArray(sendBuffer.data(), sendCounts.data(),
      sendOffsets.data(), recvBuffer.data(), recvCounts.data(), recvOffsets.data(), VTK_ID_TYPE);

  if (!result)
  {
    vtkErrorWithObjectMacro(communicator, "vtkCommunicator::AllToAllVVoidArray method failed");
    return EXIT_FAILURE;
  }

  // Use MPI function of AllToAllVVoidArray
  communicator->AllToAllVVoidArray(sendBuffer.data(), sendCounts.data(), sendOffsets.data(),
    recvBufferMPI.data(), recvCounts.data(), recvOffsets.data(), VTK_ID_TYPE);

  std::vector<vtkIdType> trueRecvBuffer(totalRecv);

  // The first process will receive 0 10 20 30
  // The second process will receive 1 1 11 11 21 21 31 31 etc.
  for (int process = 0; process < numberOfProcesses; ++process)
  {
    for (int i = 0; i < processId + 1; ++i)
    {
      trueRecvBuffer[recvOffsets[process] + i] = 10 * process + processId;
    }
  }

  // Check if each processor receive the correct buffer and if MPI function has the same result than
  // our custom function
  for (int i = 0; i < totalRecv; ++i)
  {
    if (recvBuffer[i] != recvBufferMPI[i] || recvBuffer[i] != trueRecvBuffer[i])
    {
      vtkErrorWithObjectMacro(nullptr, "recvBuffer is not as expected");
      return false;
    }
  }

  return true;
}

bool TestNoDataToOneProcess(vtkMultiProcessController* controller)
{
  int numberOfProcesses = controller->GetNumberOfProcesses();
  auto communicator = controller->GetCommunicator();

  int processId = communicator->GetLocalProcessId();
  int targetProcessWithNoData = 1;

  // Send one data to each processor
  std::vector<int> sendCounts(numberOfProcesses, 1);
  std::vector<int> sendOffsets(numberOfProcesses);
  std::vector<int> recvCounts(numberOfProcesses);
  std::vector<int> recvOffsets(numberOfProcesses);

  // The process 1 will not send any data and the other process won't send any data to process 1
  if (processId == targetProcessWithNoData)
  {
    std::fill(sendCounts.begin(), sendCounts.end(), 0);
  }
  else
  {
    sendCounts[targetProcessWithNoData] = 0;
  }

  int totalSend = 0;
  for (int i = 0; i < numberOfProcesses; ++i)
  {
    sendOffsets[i] = totalSend;
    totalSend += sendCounts[i];
  }

  for (int i = 0; i < numberOfProcesses; ++i)
  {
    // The process 1 won't receive any data
    if (processId == targetProcessWithNoData)
    {
      recvCounts[i] = 0;
    }
    // Other processes won't receive data from process 1
    else
    {
      recvCounts[i] = (i == targetProcessWithNoData) ? 0 : 1;
    }
  }

  int totalRecv = 0;
  for (int i = 0; i < numberOfProcesses; ++i)
  {
    recvOffsets[i] = totalRecv;
    totalRecv += recvCounts[i];
  }

  std::vector<vtkIdType> sendBuffer(totalSend);
  for (int i = 0; i < numberOfProcesses; ++i)
  {
    if (sendCounts[i] == targetProcessWithNoData)
    {
      sendBuffer[sendOffsets[i]] = 10 * processId + i;
    }
  }

  std::vector<vtkIdType> recvBuffer(totalRecv);

  bool result =
    communicator->vtkCommunicator::AllToAllVVoidArray(sendBuffer.data(), sendCounts.data(),
      sendOffsets.data(), recvBuffer.data(), recvCounts.data(), recvOffsets.data(), VTK_ID_TYPE);

  if (!result)
  {
    vtkErrorWithObjectMacro(communicator, "vtkCommunicator::AllToAllVVoidArray method failed");
    return EXIT_FAILURE;
  }

  if (processId == targetProcessWithNoData)
  {
    return recvBuffer.empty();
  }

  int expectedIndex = 0;
  for (int sender = 0; sender < numberOfProcesses; ++sender)
  {
    if (sender == targetProcessWithNoData)
    {
      continue;
    }

    vtkIdType expectedValue = 10 * sender + processId;
    if (recvBuffer[expectedIndex++] != expectedValue)
    {
      return false;
    }
  }

  return true;
}
};

int TestAllToAllVoidArray(int argc, char* argv[])
{
  // Initialize MPI
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  // This test is supposed to run on 4 processes
  if (controller->GetNumberOfProcesses() != 4)
  {
    vtkLog(WARNING, << "test run on " << controller->GetNumberOfProcesses()
                    << " ranks (4 expected). Cannot compare result");
    return EXIT_FAILURE;
  }

  bool success = true;
  success &= ::TestDifferentBufferToEachProcess(controller);
  success &= ::TestNoDataToOneProcess(controller);

  controller->Finalize();

  if (!success)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
