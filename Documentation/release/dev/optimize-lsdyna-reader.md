## vtkLSDynaReader loads time steps faster

vtkLSDynaReader now distributes the point state arrays to the parts
with one memcpy per run of consecutively used global point ids instead
of a per-point lookup, seeks past cell state sections no enabled array
consumes, and skips the dead-cell extraction at time steps where no
cell is dead yet. Loading a time step of a large d3plot database is up
to ten times faster.

The point UserID array now contains the actual user node ids on 4 byte
word databases instead of uninitialized memory, so you can track a
node over time by its id, and the Deflection array is no longer zero
after the first time step when the deformed mesh is displayed without
dead cell removal.

Note that the AddPointInformation method and the DensePointsUsed and
SparsePointsUsed nested classes were removed from the protected
interface of the vtkLSDynaPart helper class; the distribution of the
point data is now a private implementation detail.
