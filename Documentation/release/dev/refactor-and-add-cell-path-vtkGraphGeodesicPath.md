## Refactor the vtkGraphGeodesic hierarchy and add cell path computation

vtkDijkstraGraphGeodesicPath can now be used on any vtkDataSet (instead of being restricted to vtkPolyData) and the path can be computed on the cells. When computed on the cells, the path goes through cell centers. Note that the implementation specific to images, vtkDijkstraImageGeodesicPath, has been kept.

Also, the previous class hierarchy was the following: vtkGeodesicPath <- vtkGraphGeodesicPath <- vtkDijkstraGraphGeodesicPath <- vtkDijkstraImageGeodesicPath.
Because vtkDijkstraGraphGeodesicPath was extended with new features, the previous hierarchy would bring to vtkDijkstraImageGeodesicPath useless parameters and methods. Common methods between vtkDijkstraGraphGeodesicPath and vtkDijkstraImageGeodesicPath were moved up to vtkGraphGeodesicPath and the image version now inherits from vtkGraphGeodesicPath.

While this is a breaking change, the public API is still valid.
