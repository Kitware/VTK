## Multithread vtkPolyDataNormals

Most of the vtkPolyDataNormals steps have been multithreaded except for
AutoOrientNormals and Consistency. These steps could not be multithreaded
using the existing vtkSMPTools infrastructure.
