catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

## Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
set range [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
set min [lindex $range 0]
set max [lindex $range 1]
set value [expr ($min + $max) / 2.0]

set numberOfContours 5
set epsilon [expr double($max - $min) / double($numberOfContours * 10)]
set min [expr $min + $epsilon]
set max [expr $max - $epsilon]
vtkContourFilter cf
    cf SetInput [pl3d GetOutput]
    cf GenerateValues $numberOfContours $min $max
    cf UseScalarTreeOn

for {set i 1} { $i <= $numberOfContours } {incr i} {
  vtkThreshold threshold$i
    threshold$i SetInput [cf GetOutput]
    set value [expr $min + ($i - 1) * ($max - $min) / double ($numberOfContours - 1)]
    threshold$i ThresholdBetween [expr $value - $epsilon] [expr $value + $epsilon]
  vtkDataSetMapper mapper$i
    mapper$i SetInput [threshold$i GetOutput]
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
renWin SetSize 600 200

set cam1 [ren1 GetActiveCamera]
[ren1 GetActiveCamera] SetPosition -36.3762 32.3855 51.3652
[ren1 GetActiveCamera] SetFocalPoint 8.255 33.3861 29.7687
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 0 1
ren1 ResetCameraClippingRange

iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName multipleIsoThreshold.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

