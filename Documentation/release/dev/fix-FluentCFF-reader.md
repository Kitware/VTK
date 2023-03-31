## Fix multiple bugs in Fluent CFF reader

Fix issue in paraview when opening, closing, opening again a file. Improved cleaning memory consumption. Fix bugs with nodes/coords dataset name in HDF5.
Fix bugs with chunkDim, this is for compression in HDF5. Fix bugs with polyhedron cells by changing ConvexPointSet to VTK polyhedron cell.
Add test for 3D mesh and complex cell type. Improve debugging for future CFF changes. Change dependency of IO/Geometry on HDF5 using pImpl.
Change some protected virtual methods (OpenDataFile, ParseCaseFile, GetData).
