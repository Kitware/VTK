catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Demonstrate computation of cell derivatives
# Compute vorticity - show vorticity vectors as hedgehogs
source $VTK_TCL/vtkInt.tcl

# create reader and extract the velocity and temperature
vtkUnstructuredGridReader reader
    reader SetFileName "$VTK_DATA/cylFlow.vtk"
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [reader GetOutput]
    fd2ad SetInputFieldToPointDataField
    fd2ad SetScalarComponent 0 Temperature 0
    fd2ad SetVectorComponent 0 Velocity 0
    fd2ad SetVectorComponent 1 Velocity 1
    fd2ad SetVectorComponent 2 Velocity 2
    fd2ad Update; #to Support GetScalarRange call
    
# display the mesh
vtkDataSetMapper gridMapper
    gridMapper SetInput [fd2ad GetOutput]
    eval gridMapper SetScalarRange [[fd2ad GetOutput] GetScalarRange]
vtkActor gridActor
    gridActor SetMapper gridMapper

# compute derivatives of data and get the vorticity at the cell centers
vtkCellDerivatives derivs
    derivs SetInput [fd2ad GetOutput]
    derivs SetVectorModeToComputeVorticity

# get points in the center of cells so we can put vorticity glyphs there
vtkCellCenters cc
    cc SetInput [derivs GetOutput]

# some glyphs indicating vorticity
vtkMaskPoints mask
    mask SetInput [cc GetOutput]
    mask RandomModeOn
    mask SetMaximumNumberOfPoints 100
vtkHedgeHog hhog
    hhog SetInput [mask GetOutput]
vtkPolyDataMapper mapHog
    mapHog SetInput [hhog GetOutput]
    mapHog ScalarVisibilityOff
vtkActor hogActor
    hogActor SetMapper mapHog
    [hogActor GetProperty] SetColor 1 0 0

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor gridActor
ren1 AddActor hogActor

set camera [ren1 GetActiveCamera]
$camera SetClippingRange 3.10849 208.252
$camera SetFocalPoint 3.75439 -0.000334978 -1.27252
$camera SetPosition -1.18886 10.737 -11.3642
$camera SetViewUp 0.0828035 -0.662002 -0.744914


ren1 SetBackground 1 1 1
renWin SetSize 500 250

iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

renWin SetFileName "cellDerivs.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



