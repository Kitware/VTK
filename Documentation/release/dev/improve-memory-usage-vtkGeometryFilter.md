## vtkGeometryFilter: Reduce Memory Usage by 5x using vtkStaticFaceHashLinksTemplate and Improve performance

`vtkGeometryFilter` now uses 5x less memory and it does not need synchronization using `vtkStaticFaceHashLinksTemplate`.
Additionally, `vtkGeometryFilter` has better performance when dealing with 32-bit cell arrays, and it has faster
Initialize functions when dealing with Triangle and Quad Faces. Moreover, `vtkDataSet` now has a function to get the
number of faces of a cell. Lastly, the performance of `vtkUnstructuredGrid`'s IsCellBoundary/GetCellNeighbors
has been improved by reverting to an older better version, and accessing the cell array efficiently using the
Visit design patter to avoid unnecessary copies.
