package require vtk
package require vtkinteraction

#
# write to the temp directory if possible, otherwise use .
#
set dir "."
if {[info commands "rtTester"] == "rtTester"}  {
   set dir [rtTester GetTempDirectory]
}

vtkRenderer ren
vtkRenderWindow renWin
  renWin AddRenderer ren

vtkProperty property1
  property1 SetDiffuseColor 0.95 0.90 0.70

vtkMNIObjectReader reader1
  reader1 SetFileName "$VTK_DATA_ROOT/Data/mni-surface-mesh.obj"
  #reader1 SetFileName "coloured_object.obj"

vtkMNIObjectWriter writer
  writer SetInputConnection [reader1 GetOutputPort]
  writer SetFileName "$dir/mni-surface-mesh-binary.obj"
  writer SetProperty property1
  writer SetFileTypeToBinary
  writer Write

vtkMNIObjectReader reader
  reader SetFileName  "$dir/mni-surface-mesh-binary.obj"
  set property [reader GetProperty]

vtkDataSetMapper mapper
  mapper SetInputConnection [reader GetOutputPort]

vtkActor actor
  actor SetMapper mapper
  actor SetProperty $property

ren AddActor actor

vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

ren ResetCamera
[ren GetActiveCamera] Dolly 0.0
ren ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .
