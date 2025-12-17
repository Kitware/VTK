## Fix OpenGL initialization error handling and version check

VTK now properly handles OpenGL initialization errors and validates version compatibility. Previously, the OpenGL version check logic would incorrectly warn about incompatible versions even when OpenGL functions failed to initialize for other reasons.

The fix ensures that compatibility warnings only appear when the OpenGL version is actually below 3.2, rather than whenever initialization fails. This resolves issues where users would see misleading version warnings when the actual problem was unrelated to OpenGL version compatibility.

This change addresses [paraview/paraview#18549](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/12731) by correcting a regression introduced when GLAD was integrated for OpenGL function loading.
