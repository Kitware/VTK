// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "ExerciseMultiProcessController.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkProcessGroup.h"
#include <vtk_mpi.h>

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <set>

namespace
{
int mpiTag = 5678;
int data = -1;

int PROBE_TAG = 4244;

bool CheckProbing(vtkMPIController* controller)
{
  int rank = controller->GetLocalProcessId();
  int numRanks = controller->GetNumberOfProcesses();
  if (rank != 0)
  {
    controller->Send(&rank, 1, 0, PROBE_TAG);
  }
  else
  {
    std::set<int> other_ranks;
    for (int iR = 1; iR < numRanks; ++iR)
    {
      other_ranks.insert(iR);
    }

    for (int iR = 1; iR < numRanks; ++iR)
    {
      int sendingRank = -1;
      if (controller->Probe(vtkMultiProcessController::ANY_SOURCE, PROBE_TAG, &sendingRank) == 0)
      {
        std::cerr << "Probe operation failed." << std::endl;
        return false;
      }
      if (sendingRank < 0)
      {
        std::cerr << "Probe returned negative rank." << std::endl;
        return false;
      }
      auto it = other_ranks.find(sendingRank);
      if (it == other_ranks.end())
      {
        std::cerr << "Probe already received from rank " << sendingRank << std::endl;
        return false;
      }
      other_ranks.erase(it);

      int other_rank = -1;
      controller->Receive(&other_rank, 1, sendingRank, PROBE_TAG);
    }

    if (!other_ranks.empty())
    {
      std::cerr << "Did not probe all messages" << std::endl;
      return false;
    }
  }
  return true;
}

}

// returns 0 for success
int CheckNoBlockSends(vtkMPIController* controller)
{
  int retVal = 0;
  // processes send their rank to the next, higher ranked process
  int myRank = controller->GetLocalProcessId();
  int numRanks = controller->GetNumberOfProcesses();
  vtkMPICommunicator::Request sendRequest;
  data = myRank;
  if (myRank != numRanks - 1)
  {
    if (controller->NoBlockSend(&data, 1, myRank + 1, mpiTag, sendRequest) == 0)
    {
      vtkGenericWarningMacro("Problem with NoBlockSend.");
      retVal = 1;
    }
  }
  return retVal;
}

// returns 0 for success
int CheckNoBlockRecvs(
  vtkMPIController* controller, int sendSource, int wasMessageSent, const char* info)
{
  int myRank = controller->GetLocalProcessId();
  int retVal = 0;
  if (myRank)
  {
    int flag = -1, actualSource = -1, size = -1;
    if (controller->Iprobe(sendSource, mpiTag, &flag, &actualSource, &size, &size) == 0)
    {
      vtkGenericWarningMacro("Problem with Iprobe " << info);
      retVal = 1;
    }
    if (flag != wasMessageSent)
    {
      if (wasMessageSent)
      {
        vtkGenericWarningMacro("Did not receive the message yet but should have " << info);
      }
      else
      {
        vtkGenericWarningMacro(" Received a message I shouldn't have " << info);
      }
      retVal = 1;
    }
    if (wasMessageSent == 0)
    { // no message sent so no need to check if we can receive it
      return retVal;
    }
    else
    {
      if (actualSource != myRank - 1)
      {
        vtkGenericWarningMacro("Did not receive the proper source id " << info);
        retVal = 1;
      }
      if (size != 1)
      {
        vtkGenericWarningMacro("Did not receive the proper size " << info);
        retVal = 1;
      }
    }
    int recvData = -1;
    vtkMPICommunicator::Request recvRequest;
    if (controller->NoBlockReceive(&recvData, 1, sendSource, mpiTag, recvRequest) == 0)
    {
      vtkGenericWarningMacro("Problem with NoBlockReceive " << info);
      retVal = 1;
    }
    recvRequest.Wait();
    if (recvData != myRank - 1)
    {
      vtkGenericWarningMacro("Did not receive the proper information " << info);
      retVal = 1;
    }
  }
  return retVal;
}

// returns 0 for success
int ExerciseNoBlockCommunications(vtkMPIController* controller)
{
  if (controller->GetNumberOfProcesses() == 1)
  {
    return 0;
  }
  int retVal = CheckNoBlockRecvs(controller, vtkMultiProcessController::ANY_SOURCE, 0, "case 1");

  // barrier to make sure there's really no message to receive
  controller->Barrier();

  retVal = retVal | CheckNoBlockSends(controller);

  // barrier to make sure it's really a non-blocking send
  controller->Barrier();

  int myRank = controller->GetLocalProcessId();
  retVal = retVal | CheckNoBlockRecvs(controller, myRank - 1, 1, "case 2");

  // do it again with doing an any_source receive
  controller->Barrier();

  retVal = retVal | CheckNoBlockSends(controller);

  // barrier to make sure it's really a non-blocking send
  controller->Barrier();

  retVal =
    retVal | CheckNoBlockRecvs(controller, vtkMultiProcessController::ANY_SOURCE, 1, "case 3");

  return retVal;
}

int MPIController(int argc, char* argv[])
{
  // This is here to avoid false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);
  vtkLogger::Init(argc, argv);

  VTK_CREATE(vtkMPIController, controller);
  controller->Initialize(&argc, &argv, 1);

  vtkLogger::SetThreadName("rank: " + std::to_string(controller->GetLocalProcessId()));
  int retval = ExerciseMultiProcessController(controller);

  retval = retval | ExerciseNoBlockCommunications(controller);

  // The previous run of ExerciseMultiProcessController used the native MPI
  // collective operations.  There is also a second (inefficient) implementation
  // of these within the base vtkCommunicator class.  This hack should force the
  // class to use the VTK implementation.  In practice, the collective
  // operations will probably never be used like this, but this is a convenient
  // place to test for completeness.
  VTK_CREATE(vtkProcessGroup, group);
  group->Initialize(controller);
  vtkSmartPointer<vtkMultiProcessController> genericController;
  genericController.TakeReference(
    controller->vtkMultiProcessController::CreateSubController(group));
  if (!retval)
  {
    retval = ExerciseMultiProcessController(genericController);
  }

  retval = retval | (::CheckProbing(controller) ? 0 : 1);

  controller->Finalize();

  return retval;
}
