## Improve vtkGeometryFilter

`vtkGeometryFilter` has the following improvements:

1) It utilizes the new thread-safe GetCellPoints function for `vtkPolyData` and `vtkUnstructuredGrid` which is more
   performant when the existing vtkCellArray(s) use vtkIdTypeArray, because it avoids copying the data.
2) For the case of `vtkUnstructuredGrid`/`vtkUnstructuredGridBase`, it now utilizes the `vtkDataSetSurfaceFilter`'s
   algorithm which employs a FaceMemoryPool and a FaceHashMap to detect the boundary faces and detects the boundary
   faces by marking faces that have been reinserted. The improvements over vtkDataSetSurfaceFilter's approach is 1) that
   instead of marking the duplicate faces, it deletes them, and 2) that it is multithreaded.
   1) Because of this change, the `FastMode` for `vtkUnstructuredGrid` that used the `Degree` flag is no longer
      supported.
3) For the case of 3D `vtkImageData`/`vtkRectilinearGrid`/`vtkStructuredGrid`, it now utilizes the
   `vtkDataSetSurfaceFilter`'s algorithms for blank or not-blanked data. The improvement
   over `vtkDataSetSurfaceFilter`'s approach is that it is multithreaded.
4) It has now optimized memory usage by checking the minimum id type (vtkIdType/int) of pointIds that are required to
   extract the surface.
5) It can now properly handle ghost/blanked cells/points, and especially for the case of `vtkUnstructuredGrid`
   /`vtkUnstructuredGridBase`, it has a new flag named `RemoveGhostInterfaces`. By default, `RemoveGhostInterfaces` is
   on, because when rendering unstructured data ghost interfaces should not be shown. However, there are algorithms such
   as GhostCellGenerator that need these ghost interfaces.
