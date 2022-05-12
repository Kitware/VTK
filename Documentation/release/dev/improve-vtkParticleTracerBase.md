## Improve vtkParticleTracerBase

vtkParticleTracerBase, (the base class for vtkPParticleTracerBase, vtkParticleTracer, vtkPParticleTracer,
vtkParticlePathFilter, vtkPParticlePathFilter) has the follow improvements:

1) It has been multithreaded using vtkSMPTools. Multithreading is used when there is only one MPI process, or the number
   of particles is greater than 100.
2) Its interpolator can now use either a point locator or a cell locator (default) for interpolation.
3) Instead of the _StaticMesh_ flag, it now has the _MeshOverTime_ flag which has the following values:
   1) DIFFERENT: The mesh is different over time.
   2) STATIC: The mesh is the same over time.
   3) LINEAR_TRANSFORMATION: The mesh is different over time, but it is a linear transformation of the first
      time-step's mesh.
      1) For cell locators, this flag internally makes use of the new vtkLinearTransformCellLocator. This way the
         locator is only built once in the first time-step.
      2) For point locators, this flag internally re-uses the same cell links, but rebuilds the point locator since
         there is no vtkLinearTransformPointLocator yet.
   4) SAME_TOPOLOGY: The mesh is different over time, but it preserves the same topology (same number of points/cells,
      same connectivity).
      1) For cell locators, this is equivalent to _MeshOverTime_ = DIFFERENT.
      2) For point locators, this flag internally re-uses the same cell links.

vtkTemporalInterpolatedVelocityField has been heavily refactored. Internally, vtkTemporalInterpolatedVelocityField used
to use vtkCachingInterpolatedVelocityField, which had some numerical mistakes, and it was restricted to cell locators.
Now it uses vtkCompositeInterpolatedVelocityField which has been enhanced as needed. Due to this change,
vtkTemporalInterpolatedVelocityField can now use the FindCellStrategy approach, which allows it to use a cell locator
or a point locator to find a cell. The cell locator used is now vtkStaticCellLocator instead of vtkCellLocator which has
multithreaded BuildLocator() method and caches cell bounds by default. The point locator used is vtkStaticPointLocator
which has multithreaded BuildLocator() method. Like vtkParticleTracerBase, vtkTemporalInterpolatedVelocityField also
supports MeshOverTime instead of StaticMesh.

Finally, vtkCachingInterpolatedVelocityField, vtkCellLocatorInterpolatedVelocityField,
vtkInterpolatedVelocityField has been deprecated. Instead of vtkCellLocatorInterpolatedVelocityField use
vtkCompositeInterpolatedVelocityField with a vtkCellLocatorStrategy, and instead of vtkInterpolatedVelocityField use
vtkCompositeInterpolatedVelocityField with a vtkClosestPointStrategy.
