## Vortex core extraction works on polyhedral meshes

Three defects together prevented `vtkVortexCore` from running on polyhedral
meshes such as OpenFOAM cases.

`vtkPolyhedron::GetFacePoints` returned point ids in the mesh's global id
space. `vtkCell3D` documents face points as ids local to the cell, and every
other subclass does that, so callers indexing their own per-cell arrays with
the result read the wrong points or ran out of bounds. It now renumbers
through the cell's point id map; internal consumers that want global ids use
the new protected `GetGlobalFacePoints`.

`vtkParallelVectors` sized its triangle buffer at `nPoints - 2` per face and
wrote exactly that many entries. A face reporting fewer than three points made
that unsigned count wrap, and a face with collinear vertices -- hanging nodes
along an edge, common where a mesh refines -- ear-cuts into fewer triangles
while still covering the face, so the write loop ran past the ear-cut output.

`vtkVortexCore` downcast the Jacobian gradient to `vtkDoubleArray`.
`vtkGradientFilter` follows its input type, so a float velocity field yields a
float `jacobian_prime`, the downcast produced null, and null reached the
dispatch worker unchecked.

`vtkPolyhedron::GetNumberOfFaces` no longer renumbers the face array as a side
effect. It reports a count taken from the un-renumbered faces, so the work was
never needed for the answer, and it was paid by every caller that only wanted
the face count. `vtkPolyhedron::CellBoundary` relied on that side effect and
read an empty face array on a freshly initialized cell; it now generates the
faces it needs.

`vtkPolyhedron::GetCentroid` had the same latent problem and is fixed the same
way. `vtkCell3D::IsInsideOut` calls it before touching any face accessor, so on
a freshly initialized cell its loop ran zero times.

Two tests are added. `TestPolyhedronFacePoints` builds a polyhedron whose
global point ids do not start at zero and asserts that `GetFacePoints` returns
ids local to the cell. `TestVortexCorePolyhedra` runs `vtkVortexCore` over a
polyhedral grid, which no existing test did. `TestVortexCore` gains a float
velocity case for the higher order path.
