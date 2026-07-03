// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Tests vtkMPIController::PartitionControllerByCount() as a black box: it
// only calls public API (including the public building blocks
// PartitionControllerByCount() itself is composed from --
// vtkMultiProcessController::ApportionGroupSlots(), BlockDistribute(), and
// PartitionNodesLPT()) rather than reaching into any private/internal
// helpers. Intended to be run once per rank count in [2, 8] (e.g. via
// separate ctest entries each invoking
// `mpiexec -n <N> ./TestPartitionControllerByCount`).
//
// The expected grouping is computed independently, using
// vtkMPIController::GetNumberOfSharedMemoryNodes()/GetSharedMemoryNodeId()
// to learn the real node topology (how many shared-memory domains exist
// and which one each rank belongs to), then combining the same three
// public building blocks PartitionControllerByCount() itself uses (see its
// doc comment) to reproduce its expected color. This is deliberately not
// an assumption of a single node: some machines report more than one
// shared-memory domain even when running on one host (e.g. multi-socket
// nodes where the MPI runtime splits by socket/NUMA locality), so
// hardcoding "1 node" made this test environment-dependent.

#include <vtk_mpi.h>

#include "vtkMPIController.h"
#include "vtkNew.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <vector>

namespace
{
// Bundles the per-rank node-topology facts that ExpectedColor() and
// RunCase() need, so they don't have to thread five separate parameters
// through their signatures.
struct NodeTopology
{
  int NodeIndex = 0;
  int NumberOfNodes = 0;
  std::vector<int> NodeSizes;
  int NodeRank = 0;
  int NumberOfNodeRanks = 0;
};

// Derives, from every rank's node id (as reported by
// vtkMPIController::GetSharedMemoryNodeId()), each node's rank count and
// this rank's position within its own node. Only public API
// (GetNumberOfSharedMemoryNodes()/GetSharedMemoryNodeId(), plus a plain
// AllGather) is used, so this stays a black-box check rather than reaching
// into vtkMPIController's internals.
NodeTopology DiscoverNodeTopology(vtkMPIController* controller, int me)
{
  NodeTopology topology;
  const int numProcs = controller->GetNumberOfProcesses();
  topology.NumberOfNodes = controller->GetNumberOfSharedMemoryNodes();
  topology.NodeIndex = controller->GetSharedMemoryNodeId();

  std::vector<int> allNodeIndices(numProcs);
  controller->AllGather(&topology.NodeIndex, allNodeIndices.data(), 1);

  topology.NodeSizes.assign(topology.NumberOfNodes, 0);
  for (int rank = 0; rank < numProcs; ++rank)
  {
    if (allNodeIndices[rank] == topology.NodeIndex && rank < me)
    {
      ++topology.NodeRank;
    }
    ++topology.NodeSizes[allNodeIndices[rank]];
  }
  topology.NumberOfNodeRanks = topology.NodeSizes[topology.NodeIndex];
  return topology;
}

// Computes the color (group id) this rank is expected to land in, following
// the same three-way branch documented for
// vtkMPIController::PartitionControllerByCount(), built out of the same
// public static helpers that method uses.
int ExpectedColor(int numberOfGroups, const NodeTopology& topology)
{
  if (numberOfGroups == topology.NumberOfNodes)
  {
    return topology.NodeIndex;
  }
  else if (numberOfGroups > topology.NumberOfNodes)
  {
    const std::vector<int> slotsPerNode =
      vtkMultiProcessController::ApportionGroupSlots(topology.NodeSizes, numberOfGroups);
    const int myNodeSlots = slotsPerNode[topology.NodeIndex];
    const int slotOffset =
      std::accumulate(slotsPerNode.begin(), slotsPerNode.begin() + topology.NodeIndex, 0);
    const int withinNodeColor = vtkMultiProcessController::BlockDistribute(
      topology.NodeRank, topology.NumberOfNodeRanks, myNodeSlots);
    return slotOffset + withinNodeColor;
  }
  else
  {
    const std::vector<int> groupOfNode =
      vtkMultiProcessController::PartitionNodesLPT(topology.NodeSizes, numberOfGroups);
    return groupOfNode[topology.NodeIndex];
  }
}

// Determines, for the calling rank, the world rank of its group's "leader"
// (the rank that is local process 0 within the subController returned by
// PartitionControllerByCount). Two world ranks land in the same group iff
// they report the same leader. This lets the test check group membership
// without needing any access to the internal "color" value.
int GetGroupLeaderWorldRank(vtkMultiProcessController* subController, int worldRank)
{
  int leader = (subController->GetLocalProcessId() == 0) ? worldRank : -1;
  subController->Broadcast(&leader, 1, 0);
  return leader;
}

// Runs one case: requests numberOfGroups groups, and checks that the
// resulting grouping matches the expected grouping for the real,
// independently-discovered node topology.
bool RunCase(vtkMPIController* controller, int numProcs, int me, int numberOfGroups,
  const NodeTopology& topology)
{
  vtkMultiProcessController* subController = controller->PartitionControllerByCount(numberOfGroups);
  if (!subController)
  {
    std::cerr << "Rank " << me << ": PartitionControllerByCount(" << numberOfGroups
              << ") returned nullptr\n";
    return false;
  }

  const int myLeader = ::GetGroupLeaderWorldRank(subController, me);

  // Gather every rank's leader so rank 0 can validate the whole partition.
  std::vector<int> allLeaders(numProcs, -1);
  controller->AllGather(&myLeader, allLeaders.data(), 1);

  const int expectedGroups = (numberOfGroups < numProcs) ? numberOfGroups : numProcs;
  const int expectedColor = ::ExpectedColor(numberOfGroups, topology);

  // Gather every rank's expected color so membership can be compared
  // pairwise, the same way the observed leaders are compared.
  std::vector<int> allExpectedColors(numProcs, -1);
  controller->AllGather(&expectedColor, allExpectedColors.data(), 1);

  // Every rank checks its own membership matches the expected grouping:
  // all ranks with the same expected color must share a leader, and ranks
  // with different expected colors must not.
  bool ok = true;
  for (int other = 0; other < numProcs; ++other)
  {
    const bool sameGroupExpected = (allExpectedColors[other] == expectedColor);
    const bool sameGroupObserved = (allLeaders[other] == myLeader);
    if (sameGroupExpected != sameGroupObserved)
    {
      std::cerr << "Rank " << me << ": mismatch with rank " << other
                << " for numberOfGroups=" << numberOfGroups
                << " (expected same group=" << sameGroupExpected
                << ", observed same group=" << sameGroupObserved << ")\n";
      ok = false;
    }
  }

  // Rank 0 additionally checks the total distinct-group count.
  if (me == 0)
  {
    std::vector<int> sortedLeaders = allLeaders;
    std::sort(sortedLeaders.begin(), sortedLeaders.end());
    const int distinctGroups = static_cast<int>(
      std::unique(sortedLeaders.begin(), sortedLeaders.end()) - sortedLeaders.begin());
    if (distinctGroups != expectedGroups)
    {
      std::cerr << "numberOfGroups=" << numberOfGroups << ": expected " << expectedGroups
                << " distinct groups, observed " << distinctGroups << "\n";
      ok = false;
    }
  }

  subController->Delete();
  return ok;
}
} // anonymous namespace

int TestPartitionControllerByCount(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> contr;
  contr->Initialize();

  const int numProcs = contr->GetNumberOfProcesses();
  const int me = contr->GetLocalProcessId();

  if (numProcs < 2 || numProcs > 8)
  {
    if (me == 0)
    {
      std::cerr << "This test must be run with between 2 and 8 ranks, got " << numProcs << "\n";
    }
    contr->Finalize();
    return EXIT_FAILURE;
  }

  const NodeTopology topology = ::DiscoverNodeTopology(contr, me);

  bool ok = true;
  // Cover: 1 group (everything merged), numberOfGroups == numProcs (every
  // rank its own group), and a spread of values in between -- including,
  // whenever numProcs allows it, at least one value that does not evenly
  // divide numProcs, to force the remainder-handling path.
  std::vector<int> groupCounts = { 1, numProcs };
  for (int g = 2; g < numProcs; ++g)
  {
    groupCounts.push_back(g);
  }
  std::sort(groupCounts.begin(), groupCounts.end());
  groupCounts.erase(std::unique(groupCounts.begin(), groupCounts.end()), groupCounts.end());

  for (int numberOfGroups : groupCounts)
  {
    ok &= ::RunCase(contr, numProcs, me, numberOfGroups, topology);
  }

  // Reduce pass/fail across all ranks so every rank returns the same
  // overall result.
  int localResult = ok ? 1 : 0;
  int globalResult = 0;
  contr->AllReduce(&localResult, &globalResult, 1, vtkCommunicator::MIN_OP);

  contr->Finalize();

  return globalResult ? EXIT_SUCCESS : EXIT_FAILURE;
}
