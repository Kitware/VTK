// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDIYDataExchanger.h"

#include "vtkDIYUtilities.h"
#include "vtkDataSet.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

// clang-format off
#include "vtk_diy2.h"
// #define DIY_USE_SPDLOG
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/partners/swap.hpp)
#include VTK_DIY2(diy/algorithms.hpp)
// clang-format on

#include <functional>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDIYDataExchanger);
vtkCxxSetObjectMacro(vtkDIYDataExchanger, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkDIYDataExchanger::vtkDIYDataExchanger()
  : Controller(nullptr)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkDIYDataExchanger::~vtkDIYDataExchanger()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
bool vtkDIYDataExchanger::AllToAll(const std::vector<vtkSmartPointer<vtkDataSet>>& sendBuffer,
  const std::vector<int>& sendCounts, std::vector<vtkSmartPointer<vtkDataSet>>& recvBuffer,
  std::vector<int>& recvCounts)
{
  if (this->Controller == nullptr || (this->Controller->GetNumberOfProcesses() <= 1))
  {
    recvBuffer = sendBuffer;
    recvCounts = sendCounts;
    return true;
  }

  if (static_cast<int>(sendCounts.size()) != this->Controller->GetNumberOfProcesses())
  {
    vtkErrorMacro("`sendCounts` size (" << sendCounts.size() << ") must match the number of ranks ("
                                        << this->Controller->GetNumberOfProcesses() << ").");
    return false;
  }

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->Controller);
  assert(static_cast<int>(sendCounts.size()) == comm.size());

  std::vector<int> offsets(comm.size(), 0);
  for (int cc = 1; cc < comm.size(); ++cc)
  {
    offsets[cc] = offsets[cc - 1] + sendCounts[cc - 1];
  }
  assert((offsets.back() + sendCounts.back()) == static_cast<int>(sendBuffer.size()));

  // collect information from all ranks about who has data from whom. this helps
  // us setup links.
  std::vector<std::vector<int>> allCounts;
  diy::mpi::all_gather(comm, sendCounts, allCounts);

  using VectorOfDataSet = std::vector<vtkSmartPointer<vtkDataSet>>;
  using VectorOfVectorOfDataSet = std::vector<VectorOfDataSet>;
  using BlockT = VectorOfVectorOfDataSet;

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new BlockT()); },
    [](void* b) { delete static_cast<BlockT*>(b); });

  // note: each rank gets 1 DIY-block.
  diy::ContiguousAssigner assigner(comm.size(), comm.size());

  auto link = new diy::Link();

  // add neighbours.
  for (int gid = 0; gid < comm.size(); ++gid)
  {
    if (allCounts[comm.rank()][gid] > 0 || allCounts[gid][comm.rank()] > 0)
    {
      link->add_neighbor(diy::BlockID(gid, assigner.rank(gid)));
    }
  }

  auto block = new BlockT(comm.size());
  for (int rank = 0; rank < comm.size(); ++rank)
  {
    if (rank == comm.rank())
    {
      continue;
    }
    auto& tosend_vector = (*block)[rank];
    tosend_vector.resize(sendCounts[rank]);
    std::copy_n(
      std::next(sendBuffer.begin(), offsets[rank]), sendCounts[rank], tosend_vector.begin());
  }

  master.add(/*gid=*/comm.rank(), block, link);
  master.foreach ([](BlockT* b, const diy::Master::ProxyWithLink& cp) {
    for (const auto& neighbor : cp.link()->neighbors())
    {
      if (neighbor.gid == cp.gid())
      {
        continue;
      } // don't enqueue for self
      auto& vector_of_ds = (*b)[neighbor.gid];
      // cp.enqueue(neighbor, static_cast<int>(vector_of_ds.size()));
      for (auto& ds : vector_of_ds)
      {
        vtkLogF(TRACE, "enqueue for %d (%p)", neighbor.gid, ds.GetPointer());
        cp.enqueue<vtkDataSet*>(neighbor, ds.GetPointer());
      }
      vector_of_ds.clear();
    }
  });
  master.exchange();
  master.foreach (
    [&offsets, &sendBuffer, &sendCounts](BlockT* b, const diy::Master::ProxyWithLink& cp) {
      for (const auto& neighbor : cp.link()->neighbors())
      {
        auto& vector_of_ds = (*b)[neighbor.gid];
        if (neighbor.gid == cp.gid())
        {
          // self; push data directly from sendBuffer.
          for (int cc = offsets[cp.gid()], max = offsets[cp.gid()] + sendCounts[cp.gid()]; cc < max;
               ++cc)
          {
            vector_of_ds.push_back(sendBuffer[cc]);
          }
        }
        else
        {
          while (cp.incoming(neighbor.gid))
          {
            vtkDataSet* ptr = nullptr;
            vtkLogF(TRACE, "dequeue from  %d", neighbor.gid);
            cp.dequeue<vtkDataSet*>(neighbor, ptr);
            vector_of_ds.push_back(vtkSmartPointer<vtkDataSet>::Take(ptr));
          }
        }
      }
    });

  block = master.get<BlockT>(0);
  assert(block != nullptr && static_cast<int>(block->size()) == comm.size());

  recvBuffer.clear();
  recvCounts.clear();
  recvCounts.resize(comm.size(), 0);
  for (int rank = 0; rank < comm.size(); ++rank)
  {
    const auto& vector_of_ds = (*block)[rank];
    recvCounts[rank] = static_cast<int>(vector_of_ds.size());
    recvBuffer.insert(recvBuffer.end(), vector_of_ds.begin(), vector_of_ds.end());
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkDIYDataExchanger::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
