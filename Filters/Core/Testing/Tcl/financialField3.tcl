package require vtk
package require vtkinteraction

# demonstrate the use and manipulation of fields and use of
# vtkProgrammableDataObjectSource. This creates fields the hard way
# (as compared to reading a vtk field file), but shows you how to
# interfaceto your own raw data.

# The image should be the same as financialField.tcl

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
   set file [open "$VTK_DATA_ROOT/Data/financial.txt" r]
   set line [gets $file]
   scan $line "%*s %d" numPts
   set numLines [expr (($numPts - 1) / 8) + 1 ]

   # create the data object
   vtkFieldData field
   field AllocateArrays 4

   # read TIME_LATE - dependent variable
   while { [gets $file arrayName] == 0 } {}
   vtkFloatArray timeLate
   timeLate SetName TIME_LATE
   for {set i 0} {$i < $numLines} {incr i} {
      set line [gets $file]
      set m [scan $line "%f %f %f %f %f %f %f %f" \
	    v(0) v(1) v(2) v(3) v(4) v(5) v(6) v(7)]
      for {set j 0} {$j < $m} {incr j} {timeLate InsertNextValue $v($j)}
   }
   field AddArray timeLate

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
   field AddArray monthlyPayment

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
   field AddArray interestRate

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
   field AddArray  monthlyIncome

   [dos GetOutput] SetFieldData field
}


# Create the dataset
vtkDataObjectToDataSetFilter do2ds
    do2ds SetInputConnection [dos GetOutputPort]
    do2ds SetDataSetTypeToPolyData
    #format: component#, arrayname, arraycomp, minArrayId, maxArrayId, normalize
    do2ds DefaultNormalizeOn
    do2ds SetPointComponent 0 $xAxis 0
    do2ds SetPointComponent 1 $yAxis 0
    do2ds SetPointComponent 2 $zAxis 0
    do2ds Update

vtkRearrangeFields rf
    rf SetInputConnection [do2ds GetOutputPort]
    rf AddOperation MOVE $scalar DATA_OBJECT POINT_DATA
    rf RemoveOperation MOVE $scalar DATA_OBJECT POINT_DATA
    rf AddOperation MOVE $scalar DATA_OBJECT POINT_DATA
    rf RemoveAllOperations
    rf AddOperation MOVE $scalar DATA_OBJECT POINT_DATA
    rf Update
    set max [lindex [[[[rf GetOutput] GetPointData] GetArray $scalar] GetRange 0] 1]

vtkArrayCalculator calc
    calc SetInputConnection [rf GetOutputPort]
    calc SetAttributeModeToUsePointData
    calc SetFunction "s / $max"
    calc AddScalarVariable s $scalar 0
    calc SetResultArrayName resArray

vtkAssignAttribute aa
    aa SetInputConnection [calc GetOutputPort]
    aa Assign resArray SCALARS POINT_DATA
    aa Update

vtkRearrangeFields rf2
    rf2 SetInputConnection [aa GetOutputPort]
    rf2 AddOperation COPY SCALARS POINT_DATA DATA_OBJECT

# construct pipeline for original population
vtkGaussianSplatter popSplatter
    popSplatter SetInputConnection [rf2 GetOutputPort]
    popSplatter SetSampleDimensions 50 50 50
    popSplatter SetRadius 0.05
    popSplatter ScalarWarpingOff
vtkContourFilter popSurface
    popSurface SetInputConnection [popSplatter GetOutputPort]
    popSurface SetValue 0 0.01
vtkPolyDataMapper popMapper
    popMapper SetInputConnection [popSurface GetOutputPort]
    popMapper ScalarVisibilityOff
    popMapper ImmediateModeRenderingOn
vtkActor popActor
    popActor SetMapper popMapper
    [popActor GetProperty] SetOpacity 0.3
    [popActor GetProperty] SetColor .9 .9 .9

# construct pipeline for delinquent population
vtkGaussianSplatter lateSplatter
    lateSplatter SetInputConnection [aa GetOutputPort]
    lateSplatter SetSampleDimensions 50 50 50
    lateSplatter SetRadius 0.05
    lateSplatter SetScaleFactor 0.05
vtkContourFilter lateSurface
    lateSurface SetInputConnection [lateSplatter GetOutputPort]
    lateSurface SetValue 0 0.01
vtkPolyDataMapper lateMapper
    lateMapper SetInputConnection [lateSurface GetOutputPort]
    lateMapper ScalarVisibilityOff
vtkActor lateActor
    lateActor SetMapper lateMapper
    [lateActor GetProperty] SetColor 1.0 0.0 0.0

# create axes
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

# label the axes
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

# Graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - Field Data"
    renWin SetSize 300 300
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor axesActor
ren1 AddActor lateActor
ren1 AddActor XActor
ren1 AddActor YActor
ren1 AddActor ZActor
ren1 AddActor popActor;#it's last because its translucent
ren1 SetBackground 1 1 1

vtkCamera camera
    camera SetClippingRange .274 13.72
    camera SetFocalPoint 0.433816 0.333131 0.449
    camera SetPosition -1.96987 1.15145 1.49053
    camera SetViewUp 0.378927 0.911821 0.158107
ren1 SetActiveCamera camera
XActor SetCamera camera
YActor SetCamera camera
ZActor SetCamera camera

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
