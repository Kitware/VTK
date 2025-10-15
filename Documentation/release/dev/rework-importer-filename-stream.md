## Rework vtkImporter API to include FileName and Stream

vtkImporter now provides an API to specify FileName and Stream.

vtk3DSImporter now supports stream.
vtkOBJImporter now supports streams.

In that context the following members are now private (in the parent class):
 - vtk3DSImporter::FileName
 - vtk3DSImporter::FileFD
 - vtkGLTFImporter::FileName
 - vtkGLTFImporter::Stream
 - vtkVRMLImporter::FileName
 - vtkOBJImporter::Impl
