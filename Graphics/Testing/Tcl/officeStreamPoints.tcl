package require vtk
package require vtkinteraction

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read data
#
vtkStructuredGridReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/office.binary.vtk"
    reader Update;#force a read to occur

# to add coverage for vtkMultiPartExtentTranslator
vtkMultiPartExtentTranslator translator
[reader GetOutput] SetExtentTranslator translator

set length [[reader GetOutput] GetLength]

set maxVelocity \
  [[[[reader GetOutput] GetPointData] GetVectors] GetMaxNorm]
set maxTime [expr 35.0 * $length / $maxVelocity]

vtkStructuredGridGeometryFilter table1
    table1 SetInput [reader GetOutput]
    table1 SetExtent 11 15 7 9 8 8
vtkPolyDataMapper mapTable1
    mapTable1 SetInput [table1 GetOutput]
    mapTable1 ScalarVisibilityOff
vtkActor table1Actor
    table1Actor SetMapper mapTable1
    [table1Actor GetProperty] SetColor .59 .427 .392

vtkStructuredGridGeometryFilter table2
    table2 SetInput [reader GetOutput]
    table2 SetExtent 11 15 10 12 8 8
vtkPolyDataMapper mapTable2
    mapTable2 SetInput [table2 GetOutput]
    mapTable2 ScalarVisibilityOff
vtkActor table2Actor
    table2Actor SetMapper mapTable2
    [table2Actor GetProperty] SetColor .59 .427 .392

vtkStructuredGridGeometryFilter FilingCabinet1
    FilingCabinet1 SetInput [reader GetOutput]
    FilingCabinet1 SetExtent 15 15 7 9 0 8
vtkPolyDataMapper mapFilingCabinet1
    mapFilingCabinet1 SetInput [FilingCabinet1 GetOutput]
    mapFilingCabinet1 ScalarVisibilityOff
vtkActor FilingCabinet1Actor
    FilingCabinet1Actor SetMapper mapFilingCabinet1
    [FilingCabinet1Actor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter FilingCabinet2
    FilingCabinet2 SetInput [reader GetOutput]
    FilingCabinet2 SetExtent 15 15 10 12 0 8
vtkPolyDataMapper mapFilingCabinet2
    mapFilingCabinet2 SetInput [FilingCabinet2 GetOutput]
    mapFilingCabinet2 ScalarVisibilityOff
vtkActor FilingCabinet2Actor
    FilingCabinet2Actor SetMapper mapFilingCabinet2
    [FilingCabinet2Actor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Top
    bookshelf1Top SetInput [reader GetOutput]
    bookshelf1Top SetExtent 13 13 0 4 0 11
vtkPolyDataMapper mapBookshelf1Top
    mapBookshelf1Top SetInput [bookshelf1Top GetOutput]
    mapBookshelf1Top ScalarVisibilityOff
vtkActor bookshelf1TopActor
    bookshelf1TopActor SetMapper mapBookshelf1Top
    [bookshelf1TopActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Bottom
    bookshelf1Bottom SetInput [reader GetOutput]
    bookshelf1Bottom SetExtent 20 20 0 4 0 11
vtkPolyDataMapper mapBookshelf1Bottom
    mapBookshelf1Bottom SetInput [bookshelf1Bottom GetOutput]
    mapBookshelf1Bottom ScalarVisibilityOff
vtkActor bookshelf1BottomActor
    bookshelf1BottomActor SetMapper mapBookshelf1Bottom
    [bookshelf1BottomActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Front
    bookshelf1Front SetInput [reader GetOutput]
    bookshelf1Front SetExtent 13 20 0 0 0 11
vtkPolyDataMapper mapBookshelf1Front
    mapBookshelf1Front SetInput [bookshelf1Front GetOutput]
    mapBookshelf1Front ScalarVisibilityOff
vtkActor bookshelf1FrontActor
    bookshelf1FrontActor SetMapper mapBookshelf1Front
    [bookshelf1FrontActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Back
    bookshelf1Back SetInput [reader GetOutput]
    bookshelf1Back SetExtent 13 20 4 4 0 11
vtkPolyDataMapper mapBookshelf1Back
    mapBookshelf1Back SetInput [bookshelf1Back GetOutput]
    mapBookshelf1Back ScalarVisibilityOff
vtkActor bookshelf1BackActor
    bookshelf1BackActor SetMapper mapBookshelf1Back
    [bookshelf1BackActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1LHS
    bookshelf1LHS SetInput [reader GetOutput]
    bookshelf1LHS SetExtent 13 20 0 4 0 0
vtkPolyDataMapper mapBookshelf1LHS
    mapBookshelf1LHS SetInput [bookshelf1LHS GetOutput]
    mapBookshelf1LHS ScalarVisibilityOff
vtkActor bookshelf1LHSActor
    bookshelf1LHSActor SetMapper mapBookshelf1LHS
    [bookshelf1LHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1RHS
    bookshelf1RHS SetInput [reader GetOutput]
    bookshelf1RHS SetExtent 13 20 0 4 11 11
vtkPolyDataMapper mapBookshelf1RHS
    mapBookshelf1RHS SetInput [bookshelf1RHS GetOutput]
    mapBookshelf1RHS ScalarVisibilityOff
vtkActor bookshelf1RHSActor
    bookshelf1RHSActor SetMapper mapBookshelf1RHS
    [bookshelf1RHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Top
    bookshelf2Top SetInput [reader GetOutput]
    bookshelf2Top SetExtent 13 13 15 19 0 11
vtkPolyDataMapper mapBookshelf2Top
    mapBookshelf2Top SetInput [bookshelf2Top GetOutput]
    mapBookshelf2Top ScalarVisibilityOff
vtkActor bookshelf2TopActor
    bookshelf2TopActor SetMapper mapBookshelf2Top
    [bookshelf2TopActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Bottom
    bookshelf2Bottom SetInput [reader GetOutput]
    bookshelf2Bottom SetExtent 20 20 15 19 0 11
vtkPolyDataMapper mapBookshelf2Bottom
    mapBookshelf2Bottom SetInput [bookshelf2Bottom GetOutput]
    mapBookshelf2Bottom ScalarVisibilityOff
vtkActor bookshelf2BottomActor
    bookshelf2BottomActor SetMapper mapBookshelf2Bottom
    [bookshelf2BottomActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Front
    bookshelf2Front SetInput [reader GetOutput]
    bookshelf2Front SetExtent 13 20 15 15 0 11
vtkPolyDataMapper mapBookshelf2Front
    mapBookshelf2Front SetInput [bookshelf2Front GetOutput]
    mapBookshelf2Front ScalarVisibilityOff
vtkActor bookshelf2FrontActor
    bookshelf2FrontActor SetMapper mapBookshelf2Front
    [bookshelf2FrontActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Back
    bookshelf2Back SetInput [reader GetOutput]
    bookshelf2Back SetExtent 13 20 19 19 0 11
vtkPolyDataMapper mapBookshelf2Back
    mapBookshelf2Back SetInput [bookshelf2Back GetOutput]
    mapBookshelf2Back ScalarVisibilityOff
vtkActor bookshelf2BackActor
    bookshelf2BackActor SetMapper mapBookshelf2Back
    [bookshelf2BackActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2LHS
    bookshelf2LHS SetInput [reader GetOutput]
    bookshelf2LHS SetExtent 13 20 15 19 0 0
vtkPolyDataMapper mapBookshelf2LHS
    mapBookshelf2LHS SetInput [bookshelf2LHS GetOutput]
    mapBookshelf2LHS ScalarVisibilityOff
vtkActor bookshelf2LHSActor
    bookshelf2LHSActor SetMapper mapBookshelf2LHS
    [bookshelf2LHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2RHS
    bookshelf2RHS SetInput [reader GetOutput]
    bookshelf2RHS SetExtent 13 20 15 19 11 11
vtkPolyDataMapper mapBookshelf2RHS
    mapBookshelf2RHS SetInput [bookshelf2RHS GetOutput]
    mapBookshelf2RHS ScalarVisibilityOff
vtkActor bookshelf2RHSActor
    bookshelf2RHSActor SetMapper mapBookshelf2RHS
    [bookshelf2RHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter window
    window SetInput [reader GetOutput]
    window SetExtent 20 20 6 13 10 13
vtkPolyDataMapper mapWindow
    mapWindow SetInput [window GetOutput]
    mapWindow ScalarVisibilityOff
vtkActor windowActor
    windowActor SetMapper mapWindow
    [windowActor GetProperty] SetColor .3 .3 .5

vtkStructuredGridGeometryFilter outlet
    outlet SetInput [reader GetOutput]
    outlet SetExtent 0 0 9 10 14 16
vtkPolyDataMapper mapOutlet
    mapOutlet SetInput [outlet GetOutput]
    mapOutlet ScalarVisibilityOff
vtkActor outletActor
    outletActor SetMapper mapOutlet
    [outletActor GetProperty] SetColor 0 0 0

vtkStructuredGridGeometryFilter inlet
    inlet SetInput [reader GetOutput]
    inlet SetExtent 0 0 9 10 0 6
vtkPolyDataMapper mapInlet
    mapInlet SetInput [inlet GetOutput]
    mapInlet ScalarVisibilityOff
vtkActor inletActor
    inletActor SetMapper mapInlet
    [inletActor GetProperty] SetColor 0 0 0

vtkStructuredGridOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyDataMapper mapOutline
    mapOutline SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper mapOutline
    [outlineActor GetProperty] SetColor 0 0 0

# Create source for streamtubes
vtkStreamPoints streamer
    streamer SetInput [reader GetOutput]
    streamer SetStartPosition 0.1 2.1 0.5
    streamer SetMaximumPropagationTime 500
    streamer SetTimeIncrement 0.5
    streamer SetIntegrationDirectionToForward

vtkConeSource cone
    cone SetResolution 8
vtkGlyph3D cones
    cones SetInput [streamer GetOutput]
    cones SetSource [cone GetOutput]
    cones SetScaleFactor 0.5
    cones SetScaleModeToScaleByVector
vtkPolyDataMapper mapCones
    mapCones SetInput [cones GetOutput]
    eval mapCones SetScalarRange [[reader GetOutput] GetScalarRange]
vtkActor conesActor
    conesActor SetMapper mapCones

ren1 AddActor table1Actor
ren1 AddActor table2Actor
ren1 AddActor FilingCabinet1Actor
ren1 AddActor FilingCabinet2Actor
ren1 AddActor bookshelf1TopActor
ren1 AddActor bookshelf1BottomActor
ren1 AddActor bookshelf1FrontActor
ren1 AddActor bookshelf1BackActor
ren1 AddActor bookshelf1LHSActor
ren1 AddActor bookshelf1RHSActor
ren1 AddActor bookshelf2TopActor
ren1 AddActor bookshelf2BottomActor
ren1 AddActor bookshelf2FrontActor
ren1 AddActor bookshelf2BackActor
ren1 AddActor bookshelf2LHSActor
ren1 AddActor bookshelf2RHSActor
ren1 AddActor windowActor
ren1 AddActor outletActor
ren1 AddActor inletActor
ren1 AddActor outlineActor
ren1 AddActor conesActor

ren1 SetBackground 0.4 0.4 0.5

vtkCamera aCamera
    aCamera SetClippingRange 0.7724 39
    aCamera SetFocalPoint 1.14798 3.08416 2.47187
    aCamera SetPosition -2.64683 -3.55525 3.55848
    aCamera SetViewUp 0.0511273 0.132773 0.989827
    aCamera SetViewAngle 15.5033

ren1 SetActiveCamera aCamera

renWin SetSize 500 300
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# interact with data
wm withdraw .

