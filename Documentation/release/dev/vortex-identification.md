## Liutex vortex identification and lambda2 criterion

`vtkLiutex` computes the Liutex vector, its magnitude, and Omega-Liutex from a
velocity field or from a velocity gradient supplied by the solver. Liutex
measures the local rigid rotation of the fluid: its direction is the local
rotation axis, and it is zero in pure shear, where vorticity and the
Q-criterion are not. Omega-Liutex is a normalized measure between 0 and 1.

`vtkLiutexCoreLines` extracts vortex core lines, along which the gradient of the
Liutex magnitude is parallel to the Liutex vector.

Liutex was developed by Chaoqun Liu and colleagues at the University of Texas
at Arlington. The filters follow:

* C. Liu, Y. Gao, S. Tian, X. Dong, "Rortex: A new vortex vector definition and
  vorticity tensor and vector decompositions", Physics of Fluids 30, 035103
  (2018).
* Y. Wang, Y. Gao, J. Liu, C. Liu, "Explicit formula for the Liutex vector and
  physical meaning of vorticity based on the Liutex-Shear decomposition",
  Journal of Hydrodynamics 31, 464-474 (2019).
* X. Dong, Y. Gao, C. Liu, "New normalized Rortex/vortex identification
  method", Physics of Fluids 31, 011701 (2019), for Omega-Liutex.
* Y. Gao, J. Liu, Y. Yu, C. Liu, "A Liutex based definition and identification
  of vortex core center lines", Journal of Hydrodynamics 31, 445-454 (2019),
  for core lines.

`vtkGradientFilter` computes the lambda2 vortex criterion of J. Jeong and
F. Hussain, "On the identification of a vortex", Journal of Fluid Mechanics
285, 69-94 (1995), when `ComputeLambda2` is on. Vortex regions have
lambda2 < 0. `vtkmGradient` uses the VTK implementation when lambda2 is
requested.

## Vortex core criteria are computed correctly

`vtkVortexCore` computed the Q-criterion from spectral norms of the strain and
rotation tensors instead of squared Frobenius norms, and built the velocity
gradient used for the delta and lambda_ci criteria at half its value. The
criteria select which cells reach the parallel vectors step, so vortex core
lines can differ from earlier versions, and the `q-criterion`, `delta-criterion`
and `lambda_ci-criterion` arrays on the output change.

## Parallel vectors on image data and with small-valued fields

`vtkParallelVectors`, which `vtkVortexCore` and `vtkLiutexCoreLines` use,
triangulated every quadrilateral face as if its points ran around the
perimeter. `vtkVoxel` lists face points in `vtkPixel` order, so on image data
the two triangles of each face overlapped and left part of the face uncovered,
producing duplicate points and broken lines.

It also decided whether a triangle's field matrix could be inverted by
comparing the determinant with machine epsilon. That rejected well-conditioned
triangles when the field values were small and accepted numerically singular
ones when they were large, which emitted points with NaN coordinates. The test
is now relative to the size of the matrix, so the result does not depend on the
units of the fields, and points with non-finite coordinates are never emitted.
