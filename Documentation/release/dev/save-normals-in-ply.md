## Save point normals in PLY file

For complex surfaces it is not always trivial to compute the surface normals, so it is useful to store the normals in the mesh file.
PLY file format can store point normals and vtkPLYReader can read the point normals, but saving of point normals was not implemented in vtkPLYWriter.

vtkPLYWriter now saves point normals in PLY files (if point normals are present in the mesh).
