catch {load vtktcl}
catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source colors.tcl

vtkRenderMaster rm
set renWin [rm MakeRenderWindow]
set ren1 [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# read data
#
vtkStructuredGridReader reader
    reader SetFileName "../../../data/office.vtk"
    reader Update;#force a read to occur

set length [[reader GetOutput] GetLength]

set maxVelocity \
  [[[[reader GetOutput] GetPointData] GetVectors] GetMaxNorm]
set maxTime [expr 35.0 * $length / $maxVelocity]

vtkStructuredGridGeometryFilter table1
    table1 SetInput [reader GetOutput]
    table1 SetExtent 11 15 7 9 8 8
vtkPolyMapper mapTable1
    mapTable1 SetInput [table1 GetOutput]
    mapTable1 ScalarsVisibleOff
vtkActor table1Actor
    table1Actor SetMapper mapTable1
    [table1Actor GetProperty] SetColor .59 .427 .392

vtkStructuredGridGeometryFilter table2
    table2 SetInput [reader GetOutput]
    table2 SetExtent 11 15 10 12 8 8
vtkPolyMapper mapTable2
    mapTable2 SetInput [table2 GetOutput]
    mapTable2 ScalarsVisibleOff
vtkActor table2Actor
    table2Actor SetMapper mapTable2
    [table2Actor GetProperty] SetColor .59 .427 .392

vtkStructuredGridGeometryFilter FilingCabinet1
    FilingCabinet1 SetInput [reader GetOutput]
    FilingCabinet1 SetExtent 15 15 7 9 0 8
vtkPolyMapper mapFilingCabinet1
    mapFilingCabinet1 SetInput [FilingCabinet1 GetOutput]
    mapFilingCabinet1 ScalarsVisibleOff
vtkActor FilingCabinet1Actor
    FilingCabinet1Actor SetMapper mapFilingCabinet1
    [FilingCabinet1Actor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter FilingCabinet2
    FilingCabinet2 SetInput [reader GetOutput]
    FilingCabinet2 SetExtent 15 15 10 12 0 8
vtkPolyMapper mapFilingCabinet2
    mapFilingCabinet2 SetInput [FilingCabinet2 GetOutput]
    mapFilingCabinet2 ScalarsVisibleOff
vtkActor FilingCabinet2Actor
    FilingCabinet2Actor SetMapper mapFilingCabinet2
    [FilingCabinet2Actor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Top
    bookshelf1Top SetInput [reader GetOutput]
    bookshelf1Top SetExtent 13 13 0 4 0 11
vtkPolyMapper mapBookshelf1Top
    mapBookshelf1Top SetInput [bookshelf1Top GetOutput]
    mapBookshelf1Top ScalarsVisibleOff
vtkActor bookshelf1TopActor
    bookshelf1TopActor SetMapper mapBookshelf1Top
    [bookshelf1TopActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Bottom
    bookshelf1Bottom SetInput [reader GetOutput]
    bookshelf1Bottom SetExtent 20 20 0 4 0 11
vtkPolyMapper mapBookshelf1Bottom
    mapBookshelf1Bottom SetInput [bookshelf1Bottom GetOutput]
    mapBookshelf1Bottom ScalarsVisibleOff
vtkActor bookshelf1BottomActor
    bookshelf1BottomActor SetMapper mapBookshelf1Bottom
    [bookshelf1BottomActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Front
    bookshelf1Front SetInput [reader GetOutput]
    bookshelf1Front SetExtent 13 20 0 0 0 11
vtkPolyMapper mapBookshelf1Front
    mapBookshelf1Front SetInput [bookshelf1Front GetOutput]
    mapBookshelf1Front ScalarsVisibleOff
vtkActor bookshelf1FrontActor
    bookshelf1FrontActor SetMapper mapBookshelf1Front
    [bookshelf1FrontActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Back
    bookshelf1Back SetInput [reader GetOutput]
    bookshelf1Back SetExtent 13 20 4 4 0 11
vtkPolyMapper mapBookshelf1Back
    mapBookshelf1Back SetInput [bookshelf1Back GetOutput]
    mapBookshelf1Back ScalarsVisibleOff
vtkActor bookshelf1BackActor
    bookshelf1BackActor SetMapper mapBookshelf1Back
    [bookshelf1BackActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1LHS
    bookshelf1LHS SetInput [reader GetOutput]
    bookshelf1LHS SetExtent 13 20 0 4 0 0
vtkPolyMapper mapBookshelf1LHS
    mapBookshelf1LHS SetInput [bookshelf1LHS GetOutput]
    mapBookshelf1LHS ScalarsVisibleOff
vtkActor bookshelf1LHSActor
    bookshelf1LHSActor SetMapper mapBookshelf1LHS
    [bookshelf1LHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1RHS
    bookshelf1RHS SetInput [reader GetOutput]
    bookshelf1RHS SetExtent 13 20 0 4 11 11
vtkPolyMapper mapBookshelf1RHS
    mapBookshelf1RHS SetInput [bookshelf1RHS GetOutput]
    mapBookshelf1RHS ScalarsVisibleOff
vtkActor bookshelf1RHSActor
    bookshelf1RHSActor SetMapper mapBookshelf1RHS
    [bookshelf1RHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Top
    bookshelf2Top SetInput [reader GetOutput]
    bookshelf2Top SetExtent 13 13 15 19 0 11
vtkPolyMapper mapBookshelf2Top
    mapBookshelf2Top SetInput [bookshelf2Top GetOutput]
    mapBookshelf2Top ScalarsVisibleOff
vtkActor bookshelf2TopActor
    bookshelf2TopActor SetMapper mapBookshelf2Top
    [bookshelf2TopActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Bottom
    bookshelf2Bottom SetInput [reader GetOutput]
    bookshelf2Bottom SetExtent 20 20 15 19 0 11
vtkPolyMapper mapBookshelf2Bottom
    mapBookshelf2Bottom SetInput [bookshelf2Bottom GetOutput]
    mapBookshelf2Bottom ScalarsVisibleOff
vtkActor bookshelf2BottomActor
    bookshelf2BottomActor SetMapper mapBookshelf2Bottom
    [bookshelf2BottomActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Front
    bookshelf2Front SetInput [reader GetOutput]
    bookshelf2Front SetExtent 13 20 15 15 0 11
vtkPolyMapper mapBookshelf2Front
    mapBookshelf2Front SetInput [bookshelf2Front GetOutput]
    mapBookshelf2Front ScalarsVisibleOff
vtkActor bookshelf2FrontActor
    bookshelf2FrontActor SetMapper mapBookshelf2Front
    [bookshelf2FrontActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Back
    bookshelf2Back SetInput [reader GetOutput]
    bookshelf2Back SetExtent 13 20 19 19 0 11
vtkPolyMapper mapBookshelf2Back
    mapBookshelf2Back SetInput [bookshelf2Back GetOutput]
    mapBookshelf2Back ScalarsVisibleOff
vtkActor bookshelf2BackActor
    bookshelf2BackActor SetMapper mapBookshelf2Back
    [bookshelf2BackActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2LHS
    bookshelf2LHS SetInput [reader GetOutput]
    bookshelf2LHS SetExtent 13 20 15 19 0 0
vtkPolyMapper mapBookshelf2LHS
    mapBookshelf2LHS SetInput [bookshelf2LHS GetOutput]
    mapBookshelf2LHS ScalarsVisibleOff
vtkActor bookshelf2LHSActor
    bookshelf2LHSActor SetMapper mapBookshelf2LHS
    [bookshelf2LHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2RHS
    bookshelf2RHS SetInput [reader GetOutput]
    bookshelf2RHS SetExtent 13 20 15 19 11 11
vtkPolyMapper mapBookshelf2RHS
    mapBookshelf2RHS SetInput [bookshelf2RHS GetOutput]
    mapBookshelf2RHS ScalarsVisibleOff
vtkActor bookshelf2RHSActor
    bookshelf2RHSActor SetMapper mapBookshelf2RHS
    [bookshelf2RHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter window
    window SetInput [reader GetOutput]
    window SetExtent 20 20 6 13 10 13
vtkPolyMapper mapWindow
    mapWindow SetInput [window GetOutput]
    mapWindow ScalarsVisibleOff
vtkActor windowActor
    windowActor SetMapper mapWindow
    [windowActor GetProperty] SetColor .3 .3 .5

vtkStructuredGridGeometryFilter outlet
    outlet SetInput [reader GetOutput]
    outlet SetExtent 0 0 9 10 14 16
vtkPolyMapper mapOutlet
    mapOutlet SetInput [outlet GetOutput]
    mapOutlet ScalarsVisibleOff
vtkActor outletActor
    outletActor SetMapper mapOutlet
    [outletActor GetProperty] SetColor 0 0 0

vtkStructuredGridGeometryFilter inlet
    inlet SetInput [reader GetOutput]
    inlet SetExtent 0 0 9 10 0 6
vtkPolyMapper mapInlet
    mapInlet SetInput [inlet GetOutput]
    mapInlet ScalarsVisibleOff
vtkActor inletActor
    inletActor SetMapper mapInlet
    [inletActor GetProperty] SetColor 0 0 0

vtkStructuredGridOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyMapper mapOutline
    mapOutline SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper mapOutline
    [outlineActor GetProperty] SetColor 0 0 0

# Create source for streamtubes
vtkPointSource seeds
    seeds SetRadius 0.075
    eval seeds SetCenter 0.1 2.7 0.5
    seeds SetNumberOfPoints 25
vtkStreamLine streamers
    streamers SetInput [reader GetOutput]
    streamers SetSource [seeds GetOutput]
    streamers SetMaximumPropagationTime 500
    streamers SetStepLength 0.1
    streamers Update
vtkPolyMapper mapStreamers
    mapStreamers SetInput [streamers GetOutput]
    eval mapStreamers SetScalarRange \
       [[[[reader GetOutput] GetPointData] GetScalars] GetRange]
vtkActor streamersActor
    streamersActor SetMapper mapStreamers

$ren1 AddActors table1Actor
$ren1 AddActors table2Actor
$ren1 AddActors FilingCabinet1Actor
$ren1 AddActors FilingCabinet2Actor
$ren1 AddActors bookshelf1TopActor
$ren1 AddActors bookshelf1BottomActor
$ren1 AddActors bookshelf1FrontActor
$ren1 AddActors bookshelf1BackActor
$ren1 AddActors bookshelf1LHSActor
$ren1 AddActors bookshelf1RHSActor
$ren1 AddActors bookshelf2TopActor
$ren1 AddActors bookshelf2BottomActor
$ren1 AddActors bookshelf2FrontActor
$ren1 AddActors bookshelf2BackActor
$ren1 AddActors bookshelf2LHSActor
$ren1 AddActors bookshelf2RHSActor
$ren1 AddActors windowActor
$ren1 AddActors outletActor
$ren1 AddActors inletActor
$ren1 AddActors outlineActor
$ren1 AddActors streamersActor

eval $ren1 SetBackground $slate_grey

vtkCamera aCamera
    aCamera SetClippingRange 0.726079 36.3039
    aCamera SetFocalPoint 2.43584 2.15046 1.11104
    aCamera SetPosition -4.76183 -10.4426 3.17203
    aCamera CalcViewPlaneNormal
    aCamera SetViewUp 0.0511273 0.132773 0.989827
    aCamera SetViewAngle 18.604
    aCamera Zoom 1.2

$ren1 SetActiveCamera aCamera

$renWin SetSize 500 300
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize
#$renWin SetFileName "office.tcl.ppm"
#$renWin SaveImageAsPPM

# interact with data
wm withdraw .

