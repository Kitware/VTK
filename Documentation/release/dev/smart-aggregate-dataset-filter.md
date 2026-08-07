## vtkAggregateDataSetFilter: Node-aware process grouping

`vtkAggregateDataSetFilter` previously grouped ranks into
`NumberOfTargetProcesses` groups purely by contiguous rank id, which could
leave multiple receiving (memory-aggregating) ranks clustered on the same
node instead of spread across the job. This could cause unbalanced memory
use across nodes, increased inter-node network traffic (data has to
travel off-node to reach a receiver instead of staying local), and
network contention on a node that receives from several senders on other
nodes at once.

Grouping is now delegated to a new virtual,
`vtkMultiProcessController::PartitionControllerByCount(int numberOfGroups)`:

- The base class implementation preserves the old behavior: a contiguous,
  as-even-as-possible block distribution of rank ids across groups.
- `vtkMPIController` overrides it to be node-aware. Node membership is
  discovered via `MPI_Comm_split_type(MPI_COMM_TYPE_SHARED)` and exposed
  through two new public methods, `GetNumberOfSharedMemoryNodes()` and
  `GetSharedMemoryNodeId()`. Using that topology, `PartitionControllerByCount()`:
    - gives each node exactly one group when `numberOfGroups` equals the
      node count;
    - packs whole nodes into groups when there are fewer groups than nodes,
      balancing total rank count per group with a
      longest-processing-time-first (LPT) heuristic (Graham, 1969): process
      nodes largest-first, always adding the next node to whichever group
      currently has the smallest total rank count;
    - otherwise gives each node a number of "slots" (one slot = one group)
      proportional to its rank count, via largest-remainder apportionment,
      and splits that node's own ranks into that many contiguous sub-groups.

  Falls back to reporting a single node (with a warning), which
  reproduces the base class' block-distribution behavior, if node
  membership can't be determined.

`vtkAggregateDataSetFilter` now just calls
`controller->PartitionControllerByCount(this->NumberOfTargetProcesses)`.

New API:

- `vtkMultiProcessController::PartitionControllerByCount()`, and the
  generic building blocks it's composed from, now public static methods --
  `BlockDistribute()` (contiguous block distribution), `ApportionGroupSlots()`
  (largest-remainder/Hamilton apportionment), and `PartitionNodesLPT()`
  (LPT multiprocessor scheduling).
- `vtkMPIController`'s override of `PartitionControllerByCount()`, plus
  its own `GetNumberOfSharedMemoryNodes()`/`GetSharedMemoryNodeId()`
  (kept MPI-specific, since the base class has no notion of process
  placement).
- `vtkMPICommunicator::SplitInitializeByType()`, wrapping
  `MPI_Comm_split_type` alongside the existing `SplitInitialize()`.

New test: `TestPartitionControllerByCount`, registered at 2 through 8 MPI
ranks. It independently recomputes the expected grouping using
`GetNumberOfSharedMemoryNodes()`/`GetSharedMemoryNodeId()` to learn the
real topology, then the same public `BlockDistribute()`/
`ApportionGroupSlots()`/`PartitionNodesLPT()` building blocks
`PartitionControllerByCount()` itself uses, rather than assuming a
single node. That matters because some machines report more than one
shared-memory domain even on a single host (e.g. multi-socket nodes split
by socket/NUMA locality).
