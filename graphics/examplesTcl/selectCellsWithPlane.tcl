catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a plane 
#
vtkPlaneSource plane
  plane SetResolution 10 10

vtkPlane halfPlane
  halfPlane SetOrigin -.13 -.03 0
  halfPlane SetNormal 1 .2 0

#
# assign scalars to points
#
plane Update
set points [[plane GetOutput] GetPoints]
set numPoints [$points GetNumberOfPoints]

vtkScalars scalars
  scalars SetNumberOfScalars $numPoints

for {set i 0} { $i < $numPoints} {incr i} {
    scalars SetScalar $i [eval halfPlane EvaluateFunction [$points GetPoint $i]]
}

plane Update
[[plane GetOutput] GetPointData] SetScalars scalars

vtkThreshold positive
  positive SetInput  [plane GetOutput]
  positive ThresholdByUpper 0.0
  positive AllScalarsOff

vtkThreshold negative
  negative SetInput  [positive GetOutput]
  negative ThresholdByLower 0.0
  negative AllScalarsOff

vtkDataSetMapper   planeMapper
  planeMapper SetInput [negative GetOutput]

vtkActor planeActor
  planeActor SetMapper planeMapper
  [planeActor GetProperty] SetDiffuseColor 0 0 0
  [planeActor GetProperty] SetRepresentationToWireframe
# Add the actors to the renderer, set the background and size
#
ren1 AddActor planeActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300

#renWin SetFileName "selectCellsWithPlane.tcl.ppm"
#renWin SaveImageAsPPM


# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


