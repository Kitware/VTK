# This example demonstrates how to use a programmable filter and how to use
# the special vtkDataSetToDataSet::GetOutput() methods

# first we load in the standard vtk packages into tcl
package require vtk
package require vtkinteraction
package require vtktesting

# 
# We create a 100 by 100 point plane to sample 
#
vtkPlaneSource plane
    plane SetXResolution 100
    plane SetYResolution 100

#
# We transform the plane by a factor of 10 on X and Y
#
vtkTransform transform
   transform Scale 10 10 1
vtkTransformPolyDataFilter transF
   transF SetInput [plane GetOutput]
   transF SetTransform transform

#
# Compute Bessel function and derivatives. We'll use a programmable filter
# for this. Note the unusual GetInput() & GetOutput() methods.
#
vtkProgrammableFilter besselF
   besselF SetInput [transF GetOutput]
   besselF SetExecuteMethod bessel

#
# The SetExecuteMethod takes a Tcl proc as an argument
# In here is where all the processing is done.
#
proc bessel {} {
   set input [besselF GetPolyDataInput]
   set numPts [$input GetNumberOfPoints]
   vtkPoints newPts
   vtkFloatArray derivs

    for {set i 0} {$i < $numPts} {incr i} {
	set x [$input GetPoint $i]
	set x0 [lindex $x 0]
	set x1 [lindex $x 1]

	set r [expr sqrt($x0*$x0 + $x1*$x1)]
	set x2 [expr exp(-$r) * cos(10.0*$r)]
	set deriv [expr -exp(-$r) * (cos(10.0*$r) + 10.0*sin(10.0*$r))]

	newPts InsertPoint $i $x0 $x1 $x2
	eval derivs InsertValue $i $deriv
    }

    [besselF GetPolyDataOutput] CopyStructure $input
    [besselF GetPolyDataOutput] SetPoints newPts
    [[besselF GetPolyDataOutput] GetPointData] SetScalars derivs

    newPts Delete; #reference counting - it's ok
    derivs Delete
}

#
# We warp the plane based on the scalar values calculated above
#
vtkWarpScalar warp
    warp SetInput [besselF GetPolyDataOutput]
    warp XYPlaneOn
    warp SetScaleFactor 0.5

#
# We create a mapper and actor as usual. In the case we adjust the 
# scalar range of the mapper to match that of the computed scalars
#
vtkPolyDataMapper mapper
    mapper SetInput [warp GetPolyDataOutput]
    eval mapper SetScalarRange [[besselF GetPolyDataOutput] GetScalarRange]
vtkActor carpet
    carpet SetMapper mapper

#
# Create the RenderWindow, Renderer
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
iren AddObserver UserEvent {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] Zoom 1.5
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

