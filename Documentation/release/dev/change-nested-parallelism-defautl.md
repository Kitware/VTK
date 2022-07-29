# Disable nested parallelism by default

In this new release of VTK, nested parallelism has been disabled by default
for all backends except TBB.  This change should increase performance most of
the time by preventing a large number of small task pool to be created. It is
still possible to enable nested parallelism when sub-task are coarse enough
using the `SetNestedParallelism` method or a `LocalScope`.
For TBB, nested parallelism is handled in a single task pool and should always
be beneficial.
