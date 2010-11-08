# This test checks netCDF CF reader for reading unstructured (p-sided) cells.

package require vtk

vtkRenderWindow renWin
renWin SetSize 400 200

#############################################################################
# Case 1: Spherical coordinates off.
# Open the file.
vtkNetCDFCFReader reader_cartesian
reader_cartesian SetFileName "$VTK_DATA_ROOT/Data/sampleGenGrid3.nc"

# Set the arrays we want to load.
reader_cartesian UpdateMetaData
reader_cartesian SetVariableArrayStatus "sample" 1
reader_cartesian SphericalCoordinatesOff

# Assign the field to scalars.
vtkAssignAttribute aa_cartesian
aa_cartesian SetInputConnection [reader_cartesian GetOutputPort]
aa_cartesian Assign "sample" "SCALARS" "CELL_DATA"

# Extract a surface that we can render.
vtkDataSetSurfaceFilter surface_cartesian
surface_cartesian SetInputConnection [aa_cartesian GetOutputPort]

vtkPolyDataMapper mapper_cartesian
mapper_cartesian SetInputConnection [surface_cartesian GetOutputPort]
mapper_cartesian SetScalarRange 100 2500

vtkActor actor_cartesian
actor_cartesian SetMapper mapper_cartesian

vtkRenderer ren_cartesian
ren_cartesian AddActor actor_cartesian
ren_cartesian SetViewport 0.0 0.0 0.5 1.0
renWin AddRenderer ren_cartesian

#############################################################################
# Case 2: Spherical coordinates on.
# Open the file.
vtkNetCDFCFReader reader_spherical
reader_spherical SetFileName "$VTK_DATA_ROOT/Data/sampleGenGrid3.nc"

# Set the arrays we want to load.
reader_spherical UpdateMetaData
reader_spherical SetVariableArrayStatus "sample" 1
reader_spherical SphericalCoordinatesOn

# Assign the field to scalars.
vtkAssignAttribute aa_spherical
aa_spherical SetInputConnection [reader_spherical GetOutputPort]
aa_spherical Assign "sample" "SCALARS" "CELL_DATA"

# Extract a surface that we can render.
vtkDataSetSurfaceFilter surface_spherical
surface_spherical SetInputConnection [aa_spherical GetOutputPort]

vtkPolyDataMapper mapper_spherical
mapper_spherical SetInputConnection [surface_spherical GetOutputPort]
mapper_spherical SetScalarRange 100 2500

vtkActor actor_spherical
actor_spherical SetMapper mapper_spherical

vtkRenderer ren_spherical
ren_spherical AddActor actor_spherical
ren_spherical SetViewport 0.5 0.0 1.0 1.0
renWin AddRenderer ren_spherical
