# Support PLY with texture coordinates parameters named (s, t)

- vtkPLYReader: The PLY reader can now read texture coordinates named (s, t).
- vtkPLYWriter: The PLY writer has a new (s, t) name possibility for texture coordinate name (using `vtkPLYWriter::SetTextureCoordinatesNameToST`).
