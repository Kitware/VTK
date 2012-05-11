package require vtk
package require vtkinteraction

#
# write to the temp directory if possible, otherwise use .
#
set dir "."
if {[info commands "rtTester"] == "rtTester"}  {
   set dir [rtTester GetTempDirectory]
}

vtkRenderer ren1
ren1 SetViewport 0 0 0.33 1
vtkRenderer ren2
ren2 SetViewport 0.33 0 0.67 1
vtkRenderer ren3
ren3 SetViewport 0.67 0 1 1
vtkRenderWindow renWin
  renWin SetSize 600 200
  renWin AddRenderer ren1
  renWin AddRenderer ren2
  renWin AddRenderer ren3
  renWin SetMultiSamples 0

vtkProperty property0
  property0 SetDiffuseColor 0.95 0.90 0.70

set filename "$VTK_DATA_ROOT/Data/mni-surface-mesh.obj"

vtkMNIObjectReader asciiReader
  set property1 [asciiReader GetProperty]

if { [asciiReader CanReadFile "$filename"] != 0 } {
  asciiReader SetFileName "$filename"
}

# this is just to remove the normals, to increase coverage,
# i.e. by forcing the writer to generate normals
vtkClipClosedSurface removeNormals
  removeNormals SetInputConnection [asciiReader GetOutputPort]

# this is to make triangle strips, also to increase coverage,
# because it forces the writer to decompose the strips
vtkStripper stripper
  stripper SetInputConnection [removeNormals GetOutputPort]

# test binary writing and reading for polygons
vtkMNIObjectWriter binaryWriter
  binaryWriter SetInputConnection [stripper GetOutputPort]
  binaryWriter SetFileName "$dir/mni-surface-mesh-binary.obj"
  binaryWriter SetProperty property0
  binaryWriter SetFileTypeToBinary
  binaryWriter Write

vtkMNIObjectReader binaryReader
  binaryReader SetFileName  "$dir/mni-surface-mesh-binary.obj"
  set property2 [binaryReader GetProperty]

# make a polyline object with color scalars
vtkCurvatures scalars
  scalars SetInputConnection [asciiReader GetOutputPort]

vtkLookupTable colors
  colors SetRange -14.5104 29.0208
  colors SetAlphaRange 1.0 1.0
  colors SetSaturationRange 1.0 1.0
  colors SetValueRange 1.0 1.0
  colors SetHueRange 0.0 1.0
  colors Build

# this is just to test using the SetMapper option of vtkMNIObjectWriter
vtkDataSetMapper mapper
  mapper SetLookupTable colors
  mapper UseLookupTableScalarRangeOn

vtkExtractEdges edges
  edges SetInputConnection [scalars GetOutputPort]

# test ascii writing and reading for lines
vtkMNIObjectWriter lineWriter
  lineWriter SetMapper mapper
  #lineWriter SetLookupTable colors
  lineWriter SetInputConnection [edges GetOutputPort]
  lineWriter SetFileName "$dir/mni-wire-mesh-ascii.obj"
  lineWriter Write

vtkMNIObjectReader lineReader
  lineReader SetFileName "$dir/mni-wire-mesh-ascii.obj"

# display all the results
vtkDataSetMapper mapper1
  mapper1 SetInputConnection [asciiReader GetOutputPort]

vtkDataSetMapper mapper2
  mapper2 SetInputConnection [binaryReader GetOutputPort]

vtkDataSetMapper mapper3
  mapper3 SetInputConnection [lineReader GetOutputPort]

vtkActor actor1
  actor1 SetMapper mapper1
  actor1 SetProperty $property1

vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetProperty $property2

vtkActor actor3
  actor3 SetMapper mapper3

ren1 AddActor actor1
ren2 AddActor actor2
ren3 AddActor actor3

vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.2
ren1 ResetCameraClippingRange
ren2 ResetCamera
[ren2 GetActiveCamera] Dolly 1.2
ren2 ResetCameraClippingRange
ren3 ResetCamera
[ren3 GetActiveCamera] Dolly 1.2
ren3 ResetCameraClippingRange



iren Render


# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .
