# This example demonstrates the use of fields and use of 
# vtkProgrammableDataObjectSource. It creates fields the hard way 
# (as compared to reading a vtk field file), but shows you how to
# interface to your own raw data.

package require vtk
package require vtkinteraction

set xAxis INTEREST_RATE
set yAxis MONTHLY_PAYMENT
set zAxis MONTHLY_INCOME
set scalar TIME_LATE

# Parse an ascii file and manually create a field. Then construct a 
# dataset from the field.
vtkProgrammableDataObjectSource dos
dos SetExecuteMethod parseFile

proc parseFile {} {
   global VTK_DATA_ROOT

   # Use Tcl to read an ascii file
   set file [open "$VTK_DATA_ROOT/Data/financial.txt" r]
   set line [gets $file]
   scan $line "%*s %d" numPts
   set numLines [expr (($numPts - 1) / 8) + 1 ]

   # Get the data object's field data and allocate
   # room for 4 fields
   set fieldData [[dos GetOutput] GetFieldData]
   $fieldData AllocateArrays 4

   # read TIME_LATE - dependent variable
   # search the file until an array called TIME_LATE is found
   while { [gets $file arrayName] == 0 } {}
   # Create the corresponding float array
   vtkFloatArray timeLate
   timeLate SetName TIME_LATE
   # Read the values
   for {set i 0} {$i < $numLines} {incr i} {
      set line [gets $file]
      set m [scan $line "%f %f %f %f %f %f %f %f" \
	    v(0) v(1) v(2) v(3) v(4) v(5) v(6) v(7)]
      for {set j 0} {$j < $m} {incr j} {timeLate InsertNextValue $v($j)}
   }
   # Add the array
   $fieldData AddArray timeLate 

   # MONTHLY_PAYMENT - independent variable
   while { [gets $file arrayName] == 0 } {}
   vtkFloatArray monthlyPayment
   monthlyPayment SetName MONTHLY_PAYMENT
   for {set i 0} {$i < $numLines} {incr i} {
      set line [gets $file]
      set m [scan $line "%f %f %f %f %f %f %f %f" \
	    v(0) v(1) v(2) v(3) v(4) v(5) v(6) v(7)]
      for {set j 0} {$j < $m} {incr j} {monthlyPayment InsertNextValue $v($j)}
   }
   $fieldData AddArray monthlyPayment 

   # UNPAID_PRINCIPLE - skip
   while { [gets $file arrayName] == 0 } {}
   for {set i 0} {$i < $numLines} {incr i} {
      set line [gets $file]
   }

   # LOAN_AMOUNT - skip
   while { [gets $file arrayName] == 0 } {}
   for {set i 0} {$i < $numLines} {incr i} {
      set line [gets $file]
   }

   # INTEREST_RATE - independnet variable
   while { [gets $file arrayName] == 0 } {}
   vtkFloatArray interestRate
   interestRate SetName INTEREST_RATE
   for {set i 0} {$i < $numLines} {incr i} {
      set line [gets $file]
      set m [scan $line "%f %f %f %f %f %f %f %f" \
	    v(0) v(1) v(2) v(3) v(4) v(5) v(6) v(7)]
      for {set j 0} {$j < $m} {incr j} {interestRate InsertNextValue $v($j)}
   }
   $fieldData AddArray interestRate 

   # MONTHLY_INCOME - independent variable
   while { [gets $file arrayName] == 0 } {}
   vtkIntArray monthlyIncome
   monthlyIncome SetName MONTHLY_INCOME
   for {set i 0} {$i < $numLines} {incr i} {
      set line [gets $file]
      set m [scan $line "%d %d %d %d %d %d %d %d" \
	    v(0) v(1) v(2) v(3) v(4) v(5) v(6) v(7)]
      for {set j 0} {$j < $m} {incr j} {monthlyIncome InsertNextValue $v($j)}
   }
   $fieldData AddArray  monthlyIncome 

}

# Create the dataset.
# DataObjectToDataSetFilter can create geometry using
# fields from DataObject's FieldData
vtkDataObjectToDataSetFilter do2ds
do2ds SetInputConnection [dos GetOutputPort]
# We are generating polygonal data
do2ds SetDataSetTypeToPolyData
do2ds DefaultNormalizeOn
# All we need is points. Assign them.
do2ds SetPointComponent 0 $xAxis 0 
do2ds SetPointComponent 1 $yAxis 0
do2ds SetPointComponent 2 $zAxis 0 

# RearrangeFields is used to move fields between DataObject's
# FieldData, PointData and CellData.
vtkRearrangeFields rf
rf SetInputConnection [do2ds GetOutputPort]
# Add an operation to "move TIME_LATE from DataObject's FieldData to
# PointData"
rf AddOperation MOVE $scalar DATA_OBJECT POINT_DATA
# Force the filter to execute. This is need to force the pipeline
# to execute so that we can find the range of the array TIME_LATE
rf Update
# Set max to the second (GetRange return [min,max]) of the "range of the 
# array called $scalar in the PointData of the output of rf"
set max [lindex [[[[rf GetOutput] GetPointData] GetArray $scalar] GetRange 0] 1]

# Use an ArrayCalculator to normalize TIME_LATE
vtkArrayCalculator calc
calc SetInputConnection [rf GetOutputPort]
# Working on point data
calc SetAttributeModeToUsePointData
# Map $scalar to s. When setting function, we can use s to
# represent the array $scalar (TIME_LATE) 
calc AddScalarVariable s $scalar 0
# Divide $scalar by $max (applies division to all components of the array)
calc SetFunction "s / $max"
# The output array will be called resArray
calc SetResultArrayName resArray

# Use AssignAttribute to make resArray the active scalar field
vtkAssignAttribute aa
aa SetInputConnection [calc GetOutputPort]
aa Assign resArray SCALARS POINT_DATA
aa Update

# construct pipeline for original population
# GaussianSplatter -> Contour -> Mapper -> Actor
vtkGaussianSplatter popSplatter
popSplatter SetInputConnection [aa GetOutputPort]
popSplatter SetSampleDimensions 50 50 50
popSplatter SetRadius 0.05
popSplatter ScalarWarpingOff

vtkContourFilter popSurface
popSurface SetInputConnection [popSplatter GetOutputPort]
popSurface SetValue 0 0.01

vtkPolyDataMapper popMapper
popMapper SetInputConnection [popSurface GetOutputPort]
popMapper ScalarVisibilityOff

vtkActor popActor
popActor SetMapper popMapper
[popActor GetProperty] SetOpacity 0.3
[popActor GetProperty] SetColor .9 .9 .9

# This is for decoration only.
proc CreateAxes {} {
    global xAxis yAxis zAxis
    # Create axes.
    popSplatter Update
    set bounds [[popSplatter GetOutput] GetBounds]
    vtkAxes axes
    axes SetOrigin [lindex $bounds 0]  [lindex $bounds 2]  [lindex $bounds 4]
    axes SetScaleFactor [expr [[popSplatter GetOutput] GetLength]/5.0]
    
    vtkTubeFilter axesTubes
    axesTubes SetInputConnection [axes GetOutputPort]
    axesTubes SetRadius [expr [axes GetScaleFactor]/25.0]
    axesTubes SetNumberOfSides 6
    
    vtkPolyDataMapper axesMapper
    axesMapper SetInputConnection [axesTubes GetOutputPort]

    vtkActor axesActor
    axesActor SetMapper axesMapper

    # Label the axes.
    vtkVectorText XText
    XText SetText $xAxis

    vtkPolyDataMapper XTextMapper
    XTextMapper SetInputConnection [XText GetOutputPort]

    vtkFollower XActor
    XActor SetMapper XTextMapper
    XActor SetScale 0.02 .02 .02
    XActor SetPosition 0.35 -0.05 -0.05
    [XActor GetProperty] SetColor 0 0 0

    vtkVectorText YText
    YText SetText $yAxis

    vtkPolyDataMapper YTextMapper
    YTextMapper SetInputConnection [YText GetOutputPort]

    vtkFollower YActor
    YActor SetMapper YTextMapper
    YActor SetScale 0.02 .02 .02
    YActor SetPosition -0.05 0.35 -0.05
    [YActor GetProperty] SetColor 0 0 0
    
    vtkVectorText ZText
    ZText SetText $zAxis

    vtkPolyDataMapper ZTextMapper
    ZTextMapper SetInputConnection [ZText GetOutputPort]
    
    vtkFollower ZActor
    ZActor SetMapper ZTextMapper
    ZActor SetScale 0.02 .02 .02
    ZActor SetPosition -0.05 -0.05 0.35
    [ZActor GetProperty] SetColor 0 0 0
}
CreateAxes

# Create the render window, renderer, interactor
vtkRenderer ren1
vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetWindowName "vtk - Field Data"
renWin SetSize 500 500

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor axesActor
ren1 AddActor XActor
ren1 AddActor YActor
ren1 AddActor ZActor
ren1 AddActor popActor;
ren1 SetBackground 1 1 1

# Set the default camera position
vtkCamera camera
camera SetClippingRange .274 13.72
camera SetFocalPoint 0.433816 0.333131 0.449
camera SetPosition -1.96987 1.15145 1.49053
camera SetViewUp 0.378927 0.911821 0.158107
ren1 SetActiveCamera camera
# Assign the camera to the followers.
XActor SetCamera camera
YActor SetCamera camera
ZActor SetCamera camera

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

