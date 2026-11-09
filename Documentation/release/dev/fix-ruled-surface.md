## vtkRuledSurfaceFilter correctly completes point walks

`vtkRuledSurfaceFilter` now tracks the number of segments consumed from each
contour in PointWalk mode. Previously, wrapping around a closed contour could
make the walk finish before all of its segments were processed, leaving missing
triangles in the generated surface. PointWalk now generates complete surfaces
when sweeping closed contours along a spline, including contours with different
numbers of points.
