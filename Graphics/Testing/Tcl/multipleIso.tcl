package require vtk
package require vtkinteraction

# get the interactor ui

## Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
set range [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
set min [lindex $range 0]
set max [lindex $range 1]
set value [expr ($min + $max) / 2.0]

vtkContourFilter cf
    cf SetInput [pl3d GetOutput]
    cf SetValue 0 $value
    cf UseScalarTreeOn

set numberOfContours 5
set epsilon [expr double($max - $min) / double($numberOfContours * 10)]
set min [expr $min + $epsilon]
set max [expr $max - $epsilon]

for {set i 1} { $i <= $numberOfContours } {incr i} {
  cf SetValue 0 [expr $min + (($i - 1) / double($numberOfContours - 1) )*($max - $min)]
  cf Update
  vtkPolyData pd$i
    pd$i CopyStructure [cf GetOutput]
    [pd$i GetPointData] DeepCopy [[cf GetOutput] GetPointData]
  vtkPolyDataMapper mapper$i
    mapper$i SetInput pd$i
    eval mapper$i SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
  vtkActor actor$i
    actor$i AddPosition 0 [expr $i * 12] 0
  actor$i SetMapper mapper$i
  ren1 AddActor actor$i
}

# Add the actors to the renderer, set the background and size
#
ren1 SetBackground .3 .3 .3
renWin SetSize 450 150

set cam1 [ren1 GetActiveCamera]
[ren1 GetActiveCamera] SetPosition -36.3762 32.3855 51.3652
[ren1 GetActiveCamera] SetFocalPoint 8.255 33.3861 29.7687
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 0 1
ren1 ResetCameraClippingRange

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .

