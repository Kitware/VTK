# This example demonstrates how to use a programmable point data filter and how to use
# the special vtkDataSetToDataSet::GetOutput() methods (i.e., see vtkWarpScalar)
catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create pipeline
#
# create plane to warp
vtkPlaneSource plane
    plane SetXResolution 25
    plane SetYResolution 25
vtkTransform transform
   transform Scale 10 10 1
vtkTransformPolyDataFilter transF
   transF SetInput [plane GetOutput]
   transF SetTransform transform

# Generate random vectors perpendicular to the plane. Also create random scalars.
# Note the unsual GetInput() & GetOutput() methods.
vtkProgrammablePointDataFilter randomF
   randomF SetInput [transF GetOutput]
   randomF SetExecuteMethod randomVectors

proc randomVectors {} {
   set input [randomF GetInput]
   set numPts [$input GetNumberOfPoints]
   vtkMath math
   vtkFloatPoints newPts
   vtkFloatScalars scalars
   vtkFloatVectors vectors

   for {set i 0} {$i < $numPts} {incr i} {
	set x [$input GetPoint $i]
	set x0 [lindex $x 0]
	set x1 [lindex $x 1]

        set s [math Random 0 1]

	scalars InsertScalar $i $s
	vectors InsertVector $i 0 0 $s
    }

    [[randomF GetOutput] GetPointData] SetScalars scalars
    [[randomF GetOutput] GetPointData] SetVectors vectors

    newPts Delete; #reference counting - it's ok
    scalars Delete
    vectors Delete
    math Delete
}

# warp plane
vtkWarpVector warp
    warp SetInput [randomF GetPolyDataOutput]
    warp SetScaleFactor 0.5

# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [warp GetPolyDataOutput]
    eval mapper SetScalarRange [[randomF GetPolyDataOutput] GetScalarRange]
vtkActor carpet
    carpet SetMapper mapper

# assign our actor to the renderer

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor carpet
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
renWin Render
#renWin SetFileName "valid/expCos.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

