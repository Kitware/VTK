/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDIYUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
// clang-format on

#include <map>

//==============================================================================
struct FieldDataBlock
{
  std::map<int, vtkSmartPointer<vtkFieldData>> FieldData;
};

//------------------------------------------------------------------------------
bool TestFieldDataExchange(vtkMultiProcessController* controller)
{
  const int rank = controller->GetLocalProcessId();

  if (rank == 0)
  {
    vtkLog(INFO, "Testing exchanging an integer array and a string array inside a field data");
  }

  int startIota = rank == 0 ? 0 : 10;

  vtkNew<vtkFieldData> fd;

  vtkNew<vtkIntArray> intArray;
  intArray->SetName("int");
  intArray->SetNumberOfValues(10);
  int* intPtr = intArray->GetPointer(0);
  std::iota(intPtr, intPtr + intArray->GetNumberOfValues(), startIota);
  fd->AddArray(intArray);

  vtkNew<vtkStringArray> stringArray;
  stringArray->SetName("string");
  stringArray->SetNumberOfValues(2);
  if (rank == 0)
  {
    stringArray->SetValue(0, "a");
    stringArray->SetValue(1, "bb");
  }
  else
  {
    stringArray->SetValue(0, "A");
    stringArray->SetValue(1, "BB");
  }
  fd->AddArray(stringArray);

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new FieldDataBlock()); },
    [](void* b) -> void { delete static_cast<FieldDataBlock*>(b); });

  vtkDIYExplicitAssigner assigner(comm, 1);

  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
  decomposer.decompose(comm.rank(), assigner, master);

  diy::all_to_all(
    master, assigner, [&master, &fd](FieldDataBlock* block, const diy::ReduceProxy& srp) {
      int myBlockId = srp.gid();
      if (srp.round() == 0)
      {
        for (int i = 0; i < srp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = srp.out_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.enqueue<vtkFieldData*>(blockId, fd);
          }
        }
      }
      else
      {
        for (int i = 0; i < static_cast<int>(srp.in_link().size()); ++i)
        {
          const diy::BlockID& blockId = srp.in_link().target(i);
          if (blockId.gid != myBlockId)
          {
            vtkFieldData* tmp = nullptr;
            srp.dequeue(blockId, tmp);
            block->FieldData.insert({ blockId.gid, tmp });
            tmp->FastDelete();
          }
        }
      }
    });

  if (rank > 1)
  {
    return true;
  }

  vtkFieldData* receivedFD = master.block<FieldDataBlock>(0)->FieldData.at(rank == 0 ? 1 : 0);

  vtkIntArray* receivedIntArray =
    vtkArrayDownCast<vtkIntArray>(receivedFD->GetAbstractArray("int"));

  if (receivedIntArray->GetNumberOfValues() != 10)
  {
    vtkLog(ERROR, "Wrong number of received integers in rank " << rank);
    return false;
  }

  for (vtkIdType id = 0; id < receivedIntArray->GetNumberOfValues(); ++id)
  {
    if (receivedIntArray->GetValue(id) != (rank == 0 ? 10 : 0) + id)
    {
      vtkLog(ERROR, "Wrong integer received in rank " << rank);
      return false;
    }
  }

  vtkStringArray* receivedStringArray =
    vtkArrayDownCast<vtkStringArray>(receivedFD->GetAbstractArray("string"));

  if (receivedStringArray->GetNumberOfValues() != 2)
  {
    vtkLog(ERROR, "Wrong number of received strings in rank " << rank);
    return false;
  }

  if (rank == 0)
  {
    if (receivedStringArray->GetValue(0) != std::string("A") ||
      receivedStringArray->GetValue(1) != std::string("BB"))
    {
      vtkLog(ERROR, "Wrong string received in rank " << rank);
    }
  }
  else if (rank == 1)
  {
    if (receivedStringArray->GetValue(0) != std::string("a") ||
      receivedStringArray->GetValue(1) != std::string("bb"))
    {
      vtkLog(ERROR, "Wrong string received in rank " << rank);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int TestDIYUtilities(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  if (controller->GetNumberOfProcesses() < 3)
  {
    vtkLogF(ERROR, "This test expects at least 2 ranks.");
    return EXIT_FAILURE;
  }

  TestFieldDataExchange(controller);

  controller->Finalize();
  return EXIT_SUCCESS;
}
