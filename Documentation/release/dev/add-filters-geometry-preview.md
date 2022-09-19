## Add Filters/GeometryPreview module

The Filters/GeometryPreview module has been added which includes filters for creating a
preview of the geometry of a dataset. As of now, it includes the `vtkPointSetToOctreeImageFilter`
, `vtkOctreeImageToPointSetFilter`, and `vtkPointSetStreamer` filters.

1) `vtkPointSetToOctreeImageFilter` can be used to convert a point set to an image with a number of points per cell
   target and an unsigned char octree cell array. Each bit of the unsigned char indicates if the point-set had a point
   close to one of the 8 corners of the cell. It can optionally also output a cell array based on an input point array.
   This array will have 1 or many components that represent different functions i.e. last value, min, max, count, sum,
   mean.
2) `vtkOctreeImageToPointSetFilter` can be used to convert an image with an unsigned char octree cell array to a point
   set. Each bit of the unsigned char indicates if the cell had a point close to one of its 8 corners. It can optionally
   also output a point array based on an input cell array. This array will have 1 of the components of the input array.
3) `vtkPointSetStreamer` can be used to stream points as buckets.
