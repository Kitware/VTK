#
# The dataset read by this exercise ("combVectors.vtk") has field data 
# associated with the pointdata, namely two vector fields. In this exercise, 
# you will convert both sets of field data into attribute data. Mappers only 
# process attribute data, not field data. So we must convert the field data to 
# attribute data in order to display it.  (You'll need to determine the "names"
# of the two vector fields in the field data.)
#
# If there is time remaining, you might consider adding a programmable filter 
# to convert the two sets of vectors into a single scalar field, representing 
# the angle between the two vector fields.
#
# You will most likely use vtkFieldDataToAttributeDataFilter, vtkHedgeHog, 
# and vtkProgrammableAttributeDataFilter.
#
package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#

# get the pressure gradient vector field
vtkPLOT3DReader pl3d_gradient
    pl3d_gradient SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d_gradient SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d_gradient SetScalarFunctionNumber 100
    pl3d_gradient SetVectorFunctionNumber 210
    pl3d_gradient Update

# get the velocity vector field
vtkPLOT3DReader pl3d_velocity
    pl3d_velocity SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d_velocity SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d_velocity SetScalarFunctionNumber 100
    pl3d_velocity SetVectorFunctionNumber 200
    pl3d_velocity Update


# contour the scalar fields
vtkContourFilter contour
contour SetInput [pl3d_gradient GetOutput]
contour SetValue 0 0.225

# probe the vector fields to get data at the contour surface
vtkProbeFilter probe_gradient
probe_gradient SetSource [pl3d_gradient GetOutput]
probe_gradient SetInput [contour GetOutput]

vtkProbeFilter probe_velocity
probe_velocity SetSource [pl3d_velocity GetOutput]
probe_velocity SetInput [contour GetOutput]


#
# To display the vector fields, we use vtkHedgeHog to create lines.
#
vtkHedgeHog velocity
velocity SetInput [probe_velocity GetOutput]
velocity SetScaleFactor 0.0015

vtkHedgeHog pressureGradient
pressureGradient SetInput [probe_gradient GetOutput]
pressureGradient SetScaleFactor 0.00002

#
# We use the ProgrammableAttributeDataFilter to compute the cosine
# of the angle between the two vector fields (i.e. the dot product 
# normalized by the product of the vector lengths).
#
#
vtkProgrammableAttributeDataFilter dotProduct
dotProduct SetInput [probe_velocity GetOutput]
dotProduct AddInput [probe_velocity GetOutput]
dotProduct AddInput [probe_gradient GetOutput]
dotProduct SetExecuteMethod ExecuteDot

proc ExecuteDot {} {
   # proc for ProgrammableAttributeDataFilter.  Note the use of "double()"
   # in the calculations.  This protects us from Tcl using ints and 
   # overflowing.

   set inputs [dotProduct GetInputList]
   set input0 [$inputs GetItem 0]
   set input1 [$inputs GetItem 1]
   set numPts [$input0 GetNumberOfPoints]

   set vectors0 [[$input0 GetPointData] GetVectors]
   set vectors1 [[$input1 GetPointData] GetVectors]

   vtkFloatArray scalars

   for {set i 0} {$i < $numPts} {incr i} {
       set v0 [$vectors0 GetTuple3 $i]
       set v1 [$vectors1 GetTuple3 $i]
       
       set v0x [lindex $v0 0]
       set v0y [lindex $v0 1]
       set v0z [lindex $v0 2]

       set v1x [lindex $v1 0]
       set v1y [lindex $v1 1]
       set v1z [lindex $v1 2]

       set l0 [expr double($v0x)*double($v0x) + double($v0y)*double($v0y) \
          + double($v0z)*double($v0z)]
       set l1 [expr double($v1x)*double($v1x) + double($v1y)*double($v1y) \
          + double($v1z)*double($v1z)]

       set l0 [expr sqrt(double($l0))]
       set l1 [expr sqrt(double($l1))]

       if {$l0 > 0.0 && $l1 > 0.0} {
	   set d [expr (double($v0x)*double($v1x) + double($v0y)*double($v1y)\
              + double($v0z)*double($v1z))/($l0*$l1)]
       } else {
	   set d 0.0
       }

       scalars InsertValue $i $d
    }

    [[dotProduct GetOutput] GetPointData] SetScalars scalars

    scalars Delete
}


#
# Create the mappers and actors.  Note the call to GetPolyDataOutput when
# setting up the mapper for the ProgrammableAttributeDataFilter
#
vtkPolyDataMapper velocityMapper
    velocityMapper SetInput [velocity GetOutput]
    velocityMapper ScalarVisibilityOff

vtkLODActor velocityActor
    velocityActor SetMapper velocityMapper
    velocityActor SetNumberOfCloudPoints 1000
    eval [velocityActor GetProperty] SetColor 1 0 0 

vtkPolyDataMapper pressureGradientMapper
    pressureGradientMapper SetInput [pressureGradient GetOutput]
    pressureGradientMapper ScalarVisibilityOff

vtkLODActor pressureGradientActor
    pressureGradientActor SetMapper pressureGradientMapper
    pressureGradientActor SetNumberOfCloudPoints 1000
    eval [pressureGradientActor GetProperty] SetColor 0 1 0

vtkPolyDataMapper dotMapper
    dotMapper SetInput [dotProduct GetPolyDataOutput]
    dotMapper SetScalarRange -1 1

vtkLODActor dotActor
    dotActor SetMapper dotMapper
    dotActor SetNumberOfCloudPoints 1000

# 
# The PLOT3DReader is used to draw the outline of the original dataset.
# 
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0 

#
# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor velocityActor
ren1 AddActor pressureGradientActor
ren1 AddActor dotActor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
#ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition -21.6807 -22.6387 35.9759
$cam1 SetViewUp -0.0158865 0.293715 0.955761

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render
renWin SetWindowName "Multidimensional Visualization Exercise"

# prevent the tk window from showing up then start the event loop
wm withdraw .


