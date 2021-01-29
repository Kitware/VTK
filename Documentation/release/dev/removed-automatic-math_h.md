## Removed math.h from vtkSetGet.h

The math.h include has been removed from vtkSetGet.h, so it may be
necessary to add cmath to source files. In general, cmath will be
a compatible replacement for math.h. Note that cmath is included
by vtkMath.h and by some other VTK headers.
