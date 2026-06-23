## vtkGLTFExporter: Export object names to glTF files


`vtkGLTFExporter` now uses an actor's object name (set via `SetObjectName()`)
for the corresponding mesh and node names in the exported glTF file.
Previously, all meshes and nodes were assigned generic enumerated names
(`mesh0`, `mesh1`, …) regardless of any name assigned to the actor. If no
object name is set, the existing default enumeration behavior is preserved.
