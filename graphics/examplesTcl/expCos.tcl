catch {load vtktcl}

# get the interactor ui
source vtkInt.tcl

# create pipeline
#
# create plane to warp
vtkPlaneSource plane
    plane SetXResolution 100
    plane SetYResolution 100
vtkTransform transform
   transform Scale 10 10 1
vtkTransformPolyFilter transF
   transF SetInput [plane GetOutput]
   transF SetTransform transform
   transF Update

# compute Bessel function and derivatives. This portion could be 
# encapsulated into source or filter object.
#
set input [transF GetOutput]
set numPts [$input GetNumberOfPoints]
vtkFloatPoints newPts
vtkFloatScalars derivs
vtkPolyData bessel
    bessel CopyStructure $input
    bessel SetPoints newPts
    [bessel GetPointData] SetScalars derivs

for {set i 0} {$i < $numPts} {incr i} {
    set x [$input GetPoint $i]
    set x0 [lindex $x 0]
    set x1 [lindex $x 1]

    set r [expr sqrt($x0*$x0 + $x1*$x1)]
    set x2 [expr exp(-$r) * cos(10.0*$r)]
    set deriv [expr -exp(-$r) * (cos(10.0*$r) + 10.0*sin(10.0*$r))]

    newPts InsertPoint $i $x0 $x1 $x2
    eval derivs InsertScalar $i $deriv
}
newPts Delete; #reference counting - it's ok
derivs Delete

# warp plane
vtkWarpScalar warp
    warp SetInput bessel
    warp XYPlaneOn
    warp SetScaleFactor 0.5

# mapper and actor
vtkDataSetMapper mapper
    mapper SetInput [warp GetOutput]
    eval mapper SetScalarRange [$input GetScalarRange]
vtkActor carpet
    carpet SetMapper mapper

# assign our actor to the renderer

# Create graphics stuff
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

$ren1 AddActor carpet
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

