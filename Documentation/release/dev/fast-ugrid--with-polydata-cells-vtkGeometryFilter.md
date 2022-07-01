## vtkGeometryFilter: Fast UnstructuredGrid version with PolyData cells

vtkGeometryFilter can now significantly expedite (x100 times) the conversion of an vtkUnstructuredGrid to vtkPolyData
if the vtkUnstructuredGrid has only either vertices, or lines, or polys, or strips. Additionally, It now reports
progress.
