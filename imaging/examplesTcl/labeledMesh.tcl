catch {load vtktcl}
# demonstrate use of point labeling

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create asphere
vtkSphereSource sphere
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn
vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Generate ids for labeling
vtkIdFilter ids
    ids SetInput [sphere GetOutput]
    ids CellIdsOff
    ids FieldDataOn

# Create labels for points
vtkSelectVisiblePoints visPts
    visPts SetInput [ids GetOutput]
    visPts SetRenderer ren1
vtkLabeledDataMapper ldm
    ldm SetInput [visPts GetOutput]
    ldm SetLabelFormat "%g"
#    ldm SetLabelModeToLabelScalars
#    ldm SetLabelModeToLabelNormals
    ldm SetLabelModeToLabelFieldData
#    ldm SetLabeledComponent 0
vtkActor2D pointLabels
    pointLabels SetMapper ldm    

# Create labels for cells
vtkCellCenters cc
    cc SetInput [ids GetOutput]
vtkSelectVisiblePoints visCells
    visCells SetInput [cc GetOutput]
    visCells SetRenderer ren1
vtkLabeledDataMapper cellMapper
    cellMapper SetInput [visCells GetOutput]
    cellMapper SetLabelFormat "%g"
#    cellMapper SetLabelModeToLabelScalars
#    cellMapper SetLabelModeToLabelNormals
    cellMapper SetLabelModeToLabelFieldData
vtkActor2D cellLabels
    cellLabels SetMapper cellMapper    
    [cellLabels GetProperty] SetColor 0 1 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 AddActor2D pointLabels
ren1 AddActor2D cellLabels

ren1 SetBackground 1 1 1
renWin SetSize 500 500
renWin Render

renWin SetFileName "labeledMesh.tcl.ppm"
renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



