catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkVolume16Reader v16
    v16 SetDataDimensions 128 128 
    [v16 GetOutput] SetOrigin 0.0 0.0 0.0
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "$VTK_DATA/headsq/half"
    v16 SetImageRange 45 45
    v16 SetDataSpacing 1.6 1.6 1.5
vtkContourFilter iso
    iso SetInput [v16 GetOutput]
    iso SetNumberOfContours 50
    set val 100
    for {set i 0} {$i < 50} {incr i} {
	iso SetValue $i $val
	incr val 20
    }

vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $black

vtkOutlineFilter outline
    outline SetInput [v16 GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
renWin SetSize 500 500
ren1 SetBackground 0.1 0.2 0.4

# prevent the tk window from showing up then start the event loop
wm withdraw .

renWin Render

iso SetNumberOfContours 60
for {} {$i < 60} {incr i} {
    iso SetValue $i $val
    incr val 20
}

renWin Render

iso SetNumberOfContours 70
for {} {$i < 70} {incr i} {
    iso SetValue $i $val
    incr val 20
}

renWin Render


#renWin SetFileName "TestChangeNumContours.tcl.ppm"
#renWin SaveImageAsPPM

iren SetUserMethod {wm deiconify .vtkInteract}






