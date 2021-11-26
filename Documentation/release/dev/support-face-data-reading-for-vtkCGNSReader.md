## Add support to read face data arrays in the CGNS reader

It is now possible to read either cell or face-centered data arrays in CGNS files describing meshes with 3D cells. This is done by considering the 3D cells or the 2D faces (e.g. 1 cell versus 6 faces for a cube), respectively.

Note that the element connectivity in the CGNS file must be defined with element type `NGON_n` to construct face-based meshes. Data arrays should then be defined with `GridLocation_t` either set to `CellCenter` or `FaceCenter`, respectively.

The location of the data to read is chosen by setting the member variable `vtkCGNSReader::DataLocation` as `vtkCGNSReader::CELL_DATA` or `vtkCGNSReader::FACE_DATA`, respectively. Note that the default value is `vtkCGNSReader::CELL_DATA` and corresponds to the previous default behavior.
