# Polygon centroid computation fixed

An issue with `vtkPolygon::ComputeCentroid()` has been fixed.

Instead of projecting polygon points to the x-y or x-z plane,
compute the center and then use a composite weighted-area
triangulation to estimate the centroid. This is robust to
non-planar polygons. The projection method would assume an
x-y projection was acceptable with even minor perturbations
from planarity for x-z polygons. This in turn would lead to
a centroid placed on an edge of the polygon rather than near
its center, causing other tests (such as `vtkPolyhedron::IsConvex()`)
to fail seemingly at random.

This also adds a planarity tolerance to the computation of the
centroid. If the polygon is significantly non-planar, the centroid
computation will fail.
