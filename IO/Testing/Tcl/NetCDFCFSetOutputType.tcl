# This test checks netCDF reader.  It uses the COARDS convention.

package require vtk

vtkRenderWindow renWin
renWin SetSize 400 400

#############################################################################
# Case 1: Image type.
# Open the file.
vtkNetCDFCFReader reader_image
reader_image SetFileName "$VTK_DATA_ROOT/Data/tos_O1_2001-2002.nc"
reader_image SetOutputTypeToImage

# Set the arrays we want to load.
reader_image UpdateMetaData
reader_image SetVariableArrayStatus "tos" 1
reader_image SphericalCoordinatesOff

vtkAssignAttribute aa_image
aa_image SetInputConnection [reader_image GetOutputPort]
aa_image Assign "tos" "SCALARS" "POINT_DATA"

vtkThreshold thresh_image
thresh_image SetInputConnection [aa_image GetOutputPort]
thresh_image ThresholdByLower 10000

vtkDataSetSurfaceFilter surface_image
surface_image SetInputConnection [thresh_image GetOutputPort]

vtkPolyDataMapper mapper_image
mapper_image SetInputConnection [surface_image GetOutputPort]
mapper_image SetScalarRange 270 310

vtkActor actor_image
actor_image SetMapper mapper_image

vtkRenderer ren_image
ren_image AddActor actor_image
ren_image SetViewport 0.0 0.0 0.5 0.5
renWin AddRenderer ren_image

#############################################################################
# Case 2: Rectilinear type.
# Open the file.
vtkNetCDFCFReader reader_rect
reader_rect SetFileName "$VTK_DATA_ROOT/Data/tos_O1_2001-2002.nc"
reader_rect SetOutputTypeToRectilinear

# Set the arrays we want to load.
reader_rect UpdateMetaData
reader_rect SetVariableArrayStatus "tos" 1
reader_rect SphericalCoordinatesOff

vtkAssignAttribute aa_rect
aa_rect SetInputConnection [reader_rect GetOutputPort]
aa_rect Assign "tos" "SCALARS" "POINT_DATA"

vtkThreshold thresh_rect
thresh_rect SetInputConnection [aa_rect GetOutputPort]
thresh_rect ThresholdByLower 10000

vtkDataSetSurfaceFilter surface_rect
surface_rect SetInputConnection [thresh_rect GetOutputPort]

vtkPolyDataMapper mapper_rect
mapper_rect SetInputConnection [surface_rect GetOutputPort]
mapper_rect SetScalarRange 270 310

vtkActor actor_rect
actor_rect SetMapper mapper_rect

vtkRenderer ren_rect
ren_rect AddActor actor_rect
ren_rect SetViewport 0.5 0.0 1.0 0.5
renWin AddRenderer ren_rect

#############################################################################
# Case 3: Structured type.
# Open the file.
vtkNetCDFCFReader reader_struct
reader_struct SetFileName "$VTK_DATA_ROOT/Data/tos_O1_2001-2002.nc"
reader_struct SetOutputTypeToStructured

# Set the arrays we want to load.
reader_struct UpdateMetaData
reader_struct SetVariableArrayStatus "tos" 1
reader_struct SphericalCoordinatesOff

vtkAssignAttribute aa_struct
aa_struct SetInputConnection [reader_struct GetOutputPort]
aa_struct Assign "tos" "SCALARS" "POINT_DATA"

vtkThreshold thresh_struct
thresh_struct SetInputConnection [aa_struct GetOutputPort]
thresh_struct ThresholdByLower 10000

vtkDataSetSurfaceFilter surface_struct
surface_struct SetInputConnection [thresh_struct GetOutputPort]

vtkPolyDataMapper mapper_struct
mapper_struct SetInputConnection [surface_struct GetOutputPort]
mapper_struct SetScalarRange 270 310

vtkActor actor_struct
actor_struct SetMapper mapper_struct

vtkRenderer ren_struct
ren_struct AddActor actor_struct
ren_struct SetViewport 0.0 0.5 0.5 1.0
renWin AddRenderer ren_struct

#############################################################################
# Case 4: Unstructured type.
# Open the file.
vtkNetCDFCFReader reader_auto
reader_auto SetFileName "$VTK_DATA_ROOT/Data/tos_O1_2001-2002.nc"
reader_auto SetOutputTypeToUnstructured

# Set the arrays we want to load.
reader_auto UpdateMetaData
reader_auto SetVariableArrayStatus "tos" 1
reader_auto SphericalCoordinatesOff

vtkAssignAttribute aa_auto
aa_auto SetInputConnection [reader_auto GetOutputPort]
aa_auto Assign "tos" "SCALARS" "POINT_DATA"

vtkThreshold thresh_auto
thresh_auto SetInputConnection [aa_auto GetOutputPort]
thresh_auto ThresholdByLower 10000

vtkDataSetSurfaceFilter surface_auto
surface_auto SetInputConnection [thresh_auto GetOutputPort]

vtkPolyDataMapper mapper_auto
mapper_auto SetInputConnection [surface_auto GetOutputPort]
mapper_auto SetScalarRange 270 310

vtkActor actor_auto
actor_auto SetMapper mapper_auto

vtkRenderer ren_auto
ren_auto AddActor actor_auto
ren_auto SetViewport 0.5 0.5 1.0 1.0
renWin AddRenderer ren_auto

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

renWin Render

# # Setup a lookup table.
# vtkLookupTable lut
# lut SetTableRange 270 310
# lut SetHueRange 0.66 0.0
# lut SetRampToLinear

# # Make pretty colors
# vtkImageMapToColors map
# map SetInputConnection [asinine GetOutputPort]
# map SetLookupTable lut
# map SetOutputFormatToRGB

# # vtkImageViewer viewer
# # viewer SetInputConnection [map GetOutputPort]
# # viewer SetColorWindow 256
# # viewer SetColorLevel 127.5

# # viewer Render

# vtkImageViewer2 viewer
# viewer SetInputConnection [map GetOutputPort]
# viewer Render