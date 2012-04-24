# This test checks netCDF reader.  It uses the COARDS convention.

package require vtk

# Open the file.
vtkNetCDFCFReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/tos_O1_2001-2002.nc"

# Set the arrays we want to load.
reader UpdateMetaData
reader SetVariableArrayStatus "tos" 1
reader SetSphericalCoordinates 0

vtkAssignAttribute aa
aa SetInputConnection [reader GetOutputPort]
aa Assign "tos" "SCALARS" "POINT_DATA"

vtkThreshold thresh
thresh SetInputConnection [aa GetOutputPort]
thresh ThresholdByLower 10000

vtkDataSetSurfaceFilter surface
surface SetInputConnection [thresh GetOutputPort]

vtkPolyDataMapper mapper
mapper SetInputConnection [surface GetOutputPort]
mapper SetScalarRange 270 310

vtkActor actor
actor SetMapper mapper

vtkRenderer ren
ren AddActor actor

vtkRenderWindow renWin
renWin SetSize 200 200
renWin AddRenderer ren

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