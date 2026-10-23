The vtkStreamTracer now support distributed seeds to be dispatched among ranks in a more VTK compatible scheme.
Before, UseLocalSeed expected points only in piece 0 for each rank. Now using vtkStreamTracer::LocalSeedDistributed allow for seeds to be distributed among pieces. The previous behavior is still available (and used by default for compatibility) using vtkSTreamTracer::LocalSeedPartition0.

The SetUseLocalSeed method now take a `UseLocalSeedSource` enum as input instead of a `int`. This is technically a breaking change. Also, the UseLocalSeedSource field has been moved as private to force encapsulation.
