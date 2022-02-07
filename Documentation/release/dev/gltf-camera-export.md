## vtkGLTFExporter Camera Placement

The `vtkGLTFExporter` now exports the correct camera
transformation matrix in the `"nodes"` attribute list
of the exported glTF file.  You may now export a scene
with `vtkGLTFExporter` and obtain the original camera
location with `vtkGLTFImporter` by calling
`importer->SetCamera(0)` (before `importer->Update()`
is called).
