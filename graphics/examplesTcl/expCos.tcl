# This example demonstrates how to use a programmable filter and how to use
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

# Compute Bessel function and derivatives. We'll use a programmable filter
# for this. Note the unsual GetInput() & GetOutput() methods.
vtkProgrammableFilter besselF
   besselF SetInput [transF GetOutput]
   besselF SetExecuteMethod bessel

proc bessel {} {
   set input [besselF GetPolyDataInput]
   set numPts [$input GetNumberOfPoints]
   vtkFloatPoints newPts
   vtkFloatScalars derivs

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

    [besselF GetPolyDataOutput] CopyStructure $input
    [besselF GetPolyDataOutput] SetPoints newPts
    [[besselF GetPolyDataOutput] GetPointData] SetScalars derivs

    newPts Delete; #reference counting - it's ok
    derivs Delete
}

# warp plane
vtkWarpScalar warp
    warp SetInput [besselF GetPolyDataOutput]
    warp XYPlaneOn
    warp SetScaleFactor 0.5

# mapper and actor
vtkPolyDataMapper mapper
    mapper SetInput [warp GetPolyDataOutput]
    eval mapper SetScalarRange [[besselF GetPolyDataOutput] GetScalarRange]
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
#renWin SetFileName "expCos.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

