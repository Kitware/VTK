## Contour of polyhedra scales with threads

`vtkContour3DLinearGrid`, and `vtkContourFilter` through it, built a new
`vtkPolygon` for every polyhedron the contour crossed when generating
triangles. Each `vtkPolygon` creates a dozen helper objects, and creating and
deleting VTK objects takes the object factory and garbage collector locks, so
the threads spent most of their time waiting on each other. On a grid of one
million polyhedra the contour took 432 ms on 12 threads and 221 ms on one.

Each thread now keeps one `vtkPolygon` for the whole contour. The same contour
takes 23 ms on 12 threads and 45 ms on one, and a grid of 3.4 million
polyhedra goes from 930 ms to 31 ms on 12 threads. The output is identical.

`vtkPolyhedronContour::ContourCell()` has a new overload that takes the
scratch polygon and id list. The overload without them is deprecated.
