# This example demonstrates how to control mapper to use cell scalars or point scalars.
catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create pipeline
#
# create sphere to color
vtkSphereSource sphere
    sphere SetThetaResolution 20
    sphere SetPhiResolution 40

# Compute random scalars (colors) for each cell
vtkProgrammableAttributeDataFilter randomColors
    randomColors SetInput [sphere GetOutput]
    randomColors SetExecuteMethod colorCells

proc colorCells {} {
    vtkMath randomColorGenerator
    set input [randomColors GetInput]
    set output [randomColors GetOutput]
    set numCells [$input GetNumberOfCells]
    vtkScalars colors
	colors SetNumberOfScalars $numCells

    for {set i 0} {$i < $numCells} {incr i} {
	colors SetScalar $i [randomColorGenerator Random 0 1]
    }

    [$output GetCellData] CopyScalarsOff
    [$output GetCellData] PassData [$input GetCellData]
    [$output GetCellData] SetScalars colors

    colors Delete; #reference counting - it's ok
    randomColorGenerator Delete
}

vtkCellDataToPointData pointScalars
    pointScalars SetInput [randomColors GetOutput]
    pointScalars PassCellDataOn

# create two spheres which render cell scalars and point scalars
vtkPolyDataMapper mapper
    mapper SetInput [pointScalars GetPolyDataOutput]
    eval mapper SetScalarRange [[randomColors GetPolyDataOutput] GetScalarRange]
    mapper SetScalarModeToUseCellData
vtkActor sphereActor
    sphereActor SetMapper mapper

vtkPolyDataMapper mapper2
    mapper2 SetInput [pointScalars GetPolyDataOutput]
    eval mapper2 SetScalarRange [[randomColors GetPolyDataOutput] GetScalarRange]
    mapper2 SetScalarModeToUsePointData
vtkActor sphereActor2
    sphereActor2 SetMapper mapper2
    sphereActor2 AddPosition 1 0 0

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor sphereActor
ren1 AddActor sphereActor2
renWin SetSize 400 250

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 2.5
renWin Render
renWin SetFileName "scalarMapping.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

