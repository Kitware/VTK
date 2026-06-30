## CONVERGE reader: consistent polyhedron face orientation

The CONVERGE CFD reader now builds polyhedra with consistently oriented faces.
Each interior face is shared by two cells but is stored once with a single
winding; the reader previously gave both cells the same winding, leaving nearly
every polyhedron with mixed inward and outward face normals. That produced holes
and broken cross sections when slicing or contouring CONVERGE polyhedral meshes.
The face is now reversed for the neighbor cell so all faces point outward,
matching what the CGNS reader already does with signed NFACE_n connectivity.
