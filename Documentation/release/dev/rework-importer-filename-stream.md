## Rework vtkImporter API to include FileName and Stream

vtkImporter now provides an API to specify FileName and Stream.

In that context the following members are now private (in the parent class):
 - vtk3DSImporter::FileName
 - vtkGLTFImporter::FileName
 - vtkGLTFImporter::Stream
 - vtkVRMLImporter::FileName
