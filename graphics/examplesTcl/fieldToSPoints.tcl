catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


## Demonstrates conversion of field data into structured points
# Output should be the same as complexV.tcl.

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create a reader and write out the field
vtkStructuredPointsReader reader
    reader SetFileName "$VTK_DATA/carotid.vtk"
vtkDataSetToDataObjectFilter ds2do
    ds2do SetInput [reader GetOutput]
vtkDataObjectWriter writer
    writer SetInput [ds2do GetOutput]
    writer SetFileName "SPtsField.vtk"
    writer Write

# create pipeline
#
# read the field
vtkDataObjectReader dor
    dor SetFileName "SPtsField.vtk"
vtkDataObjectToDataSetFilter do2ds
    do2ds SetInput [dor GetOutput]
    do2ds SetDataSetTypeToStructuredPoints
    do2ds SetDimensionsComponent Dimensions 0 
    do2ds SetOriginComponent Origin 0 
    do2ds SetSpacingComponent Spacing 0
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [do2ds GetStructuredPointsOutput]
    fd2ad SetInputFieldToDataObjectField
    fd2ad SetOutputAttributeDataToPointData
    fd2ad SetVectorComponent 0 vectors 0 
    fd2ad SetVectorComponent 1 vectors 1 
    fd2ad SetVectorComponent 2 vectors 2 
    fd2ad SetScalarComponent 0 scalars 0 

vtkHedgeHog hhog
    hhog SetInput [fd2ad GetOutput]
    hhog SetScaleFactor 0.3
vtkLookupTable lut
#    lut SetHueRange .667 0.0
    lut Build
vtkPolyDataMapper hhogMapper
    hhogMapper SetInput [hhog GetOutput]
    hhogMapper SetScalarRange 50 550
    hhogMapper SetLookupTable lut
    hhogMapper ImmediateModeRenderingOn
vtkActor hhogActor
    hhogActor SetMapper hhogMapper

vtkOutlineFilter outline
    outline SetInput [fd2ad GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
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
ren1 AddActor hhogActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
ren1 SetBackground 0.1 0.2 0.4
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
renWin Render
renWin SetFileName "fieldToSPoints.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


