# This example demonstrates how to use a programmable source and how to use
# the special vtkDataSetToDataSet::GetOutput() methods (i.e., see vtkWarpScalar)
catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create pipeline - use a programmable source to compute Bessel function and
# generate a plane of quadrilateral polygons,
#
vtkProgrammableSource besselSource
    besselSource SetExecuteMethod bessel

# Generate plane with Bessel function scalar values.
# It's interesting to compare this with vtkPlaneSource.
proc bessel {} {
    set XRes 25; set XOrigin -5.0; set XWidth 10.0
    set YRes 40; set YOrigin -5.0; set YWidth 10.0

    vtkFloatPoints newPts
    vtkCellArray newPolys
    vtkFloatScalars derivs

    # Compute points and scalars
    for {set id 0; set j 0} {$j < [expr $YRes+1]} {incr j} {
       set x1 [expr $YOrigin + ($j.0/$YRes.0)*$YWidth]
       for {set i 0} {$i < [expr $XRes+1]} {incr i; incr id} {
	  set x0 [expr $XOrigin + ($i.0/$XRes.0)*$XWidth]

	  set r [expr sqrt($x0*$x0 + $x1*$x1)]
	  set x2 [expr exp(-$r) * cos(10.0*$r)]
	  set deriv [expr -exp(-$r) * (cos(10.0*$r) + 10.0*sin(10.0*$r))]

	  newPts InsertPoint $id $x0 $x1 $x2
	  eval derivs InsertScalar $id $deriv
       }
    }

    # Generate polygon connectivity
    for {set j 0} {$j < $YRes} {incr j} {
       for {set i 0} {$i < $XRes} {incr i} {
	  newPolys InsertNextCell 4
	  set id [expr $i + $j*($XRes+1)]
	  newPolys InsertCellPoint $id
	  newPolys InsertCellPoint [expr $id + 1]
	  newPolys InsertCellPoint [expr $id + $XRes + 2]
	  newPolys InsertCellPoint [expr $id + $XRes + 1]
       }
    }

    [besselSource GetPolyDataOutput] SetPoints newPts
    [besselSource GetPolyDataOutput] SetPolys newPolys
    [[besselSource GetPolyDataOutput] GetPointData] SetScalars derivs

    newPts Delete; #reference counting - it's ok
    newPolys Delete
    derivs Delete
}

# warp plane
vtkWarpScalar warp
    warp SetInput [besselSource GetPolyDataOutput]
    warp XYPlaneOn
    warp SetScaleFactor 0.5

# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [warp GetPolyDataOutput]
    eval mapper SetScalarRange [[besselSource GetPolyDataOutput] GetScalarRange]
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

