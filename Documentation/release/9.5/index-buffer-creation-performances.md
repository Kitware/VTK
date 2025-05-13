# Improve IBO construction performances in vtkOpenGLIndexBufferObject

For the common cases of triangles-only polydata, the IBO construction time as been reduced by about 25%.
It is even more faster if the polydata cell arrays use 32bits connectivity IDs. This can be forced using the `VTK_USE_64BIT_IDS=OFF` when building, or using `vtkCellArray::SetDefaultStorageIs64Bit(false);` or `polydata->GetPolys()->ConvertTo32BitStorage()`.
