package require vtk
package require vtkinteraction
package require vtktesting

# Demonstrate the generation of a structured grid from field data. The output
# should be similar to combIso.tcl.
#
# NOTE: This test only works if the current directory is writable
#

if {[catch {set channel [open "test.tmp" "w"]}] == 0 } {
   close $channel
   file delete -force "test.tmp"

# get the interactor ui

# Create a reader and write out the field
vtkMultiBlockPLOT3DReader comb
    comb SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    comb SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    comb SetScalarFunctionNumber 100
    comb Update
    set output [[comb GetOutput] GetBlock 0]
vtkStructuredGridWriter wsg
  wsg SetInputData $output
  wsg SetFileTypeToBinary
  wsg SetFileName "combsg.vtk"
  wsg Write

vtkStructuredGridReader pl3d
  pl3d SetFileName "combsg.vtk"

vtkDataSetToDataObjectFilter ds2do
    ds2do SetInputConnection [pl3d GetOutputPort]

vtkDataObjectWriter writer
    writer SetInputConnection [ds2do GetOutputPort]
    writer SetFileName "SGridField.vtk"
    writer Write

# read the field
vtkDataObjectReader dor
    dor SetFileName "SGridField.vtk"
vtkDataObjectToDataSetFilter do2ds
    do2ds SetInputConnection [dor GetOutputPort]
    do2ds SetDataSetTypeToStructuredGrid
    do2ds SetDimensionsComponent "Dimensions" 0
    do2ds SetPointComponent 0 "Points" 0
    do2ds SetPointComponent 1 "Points" 1
    do2ds SetPointComponent 2 "Points" 2
    do2ds Update

vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInputData [do2ds GetStructuredGridOutput]
    fd2ad SetInputFieldToDataObjectField
    fd2ad SetOutputAttributeDataToPointData
    fd2ad SetVectorComponent 0 "Momentum" 0
    fd2ad SetVectorComponent 1 "Momentum" 1
    fd2ad SetVectorComponent 2 "Momentum" 2
    fd2ad SetScalarComponent 0 "Density" 0
    fd2ad Update

# create pipeline
#
vtkContourFilter iso
    iso SetInputConnection [fd2ad GetOutputPort]
    iso SetValue 0 .38
vtkPolyDataNormals normals
    normals SetInputConnection [iso GetOutputPort]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInputConnection [normals GetOutputPort]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque

vtkStructuredGridOutlineFilter outline
    outline SetInputData [fd2ad GetStructuredGridOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 250 250
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

#
# cleanup
   file delete -force "combsg.vtk"
   file delete -force "SGridField.vtk"

# prevent the tk window from showing up then start the event loop
wm withdraw .
}


