# Test the attribute to field data filter to plot the y-ccomponents of a vector
# (this example uses cell data)
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create pipeline
# Create an isosurface
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkPointDataToCellData p2c
    p2c SetInput [pl3d GetOutput]
    p2c PassPointDataOff

# extract y component of vector
vtkAttributeDataToFieldDataFilter ad2fd
    ad2fd SetInput [p2c GetOutput]
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [ad2fd GetOutput]
    fd2ad SetInputFieldToCellDataField
    fd2ad SetOutputAttributeDataToCellData
    fd2ad SetScalarComponent 0 CellVectors 1 

# cut with plane
vtkPlane plane
    eval plane SetOrigin [[pl3d GetOutput] GetCenter]
    plane SetNormal -0.287 0 0.9579
vtkCutter planeCut
    planeCut SetInput [fd2ad GetOutput]
    planeCut SetCutFunction plane
    planeCut Update

vtkPolyDataMapper cutMapper
    cutMapper SetInput [planeCut GetOutput]
    eval cutMapper SetScalarRange \
      [[[[planeCut GetOutput] GetCellData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0

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
ren1 AddActor cutActor
ren1 SetBackground 1 1 1
renWin SetSize 500 300

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 2.81 140.0
$cam1 SetFocalPoint 9.445 0.393 30.5901
$cam1 SetPosition -0.1975 -15.2366 51.9064
$cam1 SetViewUp -0.12802 0.826539 0.548129
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin SetFileName "fieldAttr2.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



