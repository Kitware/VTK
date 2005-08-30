package require vtk
package require vtkinteraction
package require vtktesting

set size 3187;#maximum number possible
#set size 100;#maximum number possible
set xAxis "INTEREST_RATE"
set yAxis "MONTHLY_PAYMENT"
set zAxis "MONTHLY_INCOME"
set scalar "TIME_LATE"

# extract data from field as a polydata (just points), then extract scalars
vtkDataObjectReader fdr
    fdr SetFileName "$VTK_DATA_ROOT/Data/financial.vtk"
vtkDataObjectToDataSetFilter do2ds
    do2ds SetInputConnection [fdr GetOutputPort]
    do2ds SetDataSetTypeToPolyData
    #format: component#, arrayname, arraycomp, minArrayId, maxArrayId, normalize
    do2ds DefaultNormalizeOn
    do2ds SetPointComponent 0 $xAxis 0 
    do2ds SetPointComponent 1 $yAxis 0 0 $size 1
    do2ds SetPointComponent 2 $zAxis 0 
    do2ds Update
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInputConnection [do2ds GetOutputPort]
    fd2ad SetInputFieldToDataObjectField
    fd2ad SetOutputAttributeDataToPointData
    fd2ad DefaultNormalizeOn
    fd2ad SetScalarComponent 0 $scalar 0 

# construct pipeline for original population
vtkGaussianSplatter popSplatter
    popSplatter SetInputConnection [fd2ad GetOutputPort]
    popSplatter SetSampleDimensions 50 50 50
    popSplatter SetRadius 0.05
    popSplatter ScalarWarpingOff
vtkMarchingContourFilter popSurface
    popSurface SetInputConnection [popSplatter GetOutputPort]
    popSurface SetValue 0 0.01
vtkPolyDataMapper popMapper
    popMapper SetInputConnection [popSurface GetOutputPort]
    popMapper ScalarVisibilityOff
vtkActor popActor
    popActor SetMapper popMapper
    [popActor GetProperty] SetOpacity 0.3
    [popActor GetProperty] SetColor .9 .9 .9

# construct pipeline for delinquent population
vtkGaussianSplatter lateSplatter
    lateSplatter SetInputConnection [fd2ad GetOutputPort]
    lateSplatter SetSampleDimensions 50 50 50
    lateSplatter SetRadius 0.05
    lateSplatter SetScaleFactor 0.05
vtkMarchingContourFilter lateSurface
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
renWin SetSize 400 400

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


