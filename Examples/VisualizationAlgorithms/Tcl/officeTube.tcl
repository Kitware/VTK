# This example demonstrates the use of a single streamline and the tube
# filter to create a streamtube.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands from Tcl. The vtkinteraction package defines
# a simple Tcl/Tk interactor widget.
#
package require vtk
package require vtkinteraction
package require vtktesting

# We read a data file the is a CFD analysis of airflow in an office (with
# ventilation and a burning cigarette). We force an update so that we
# can query the output for its length, i.e., the length of the diagonal
# of the bounding box. This is useful for normalizing the data.
#
vtkStructuredGridReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/office.binary.vtk"
    reader Update;#force a read to occur

set length [[reader GetOutput] GetLength]

set maxVelocity \
  [[[[reader GetOutput] GetPointData] GetVectors] GetMaxNorm]
set maxTime [expr 35.0 * $length / $maxVelocity]

# Now we will generate a single streamline in the data. We select the
# integration order to use (RungeKutta order 4) and associate it with
# the streamer. The start position is the position in world space where
# we want to begin streamline integration; and we integrate in both
# directions. The step length is the length of the line segments that
# make up the streamline (i.e., related to display). The
# IntegrationStepLength specifies the integration step length as a
# fraction of the cell size that the streamline is in.
vtkRungeKutta4 integ
vtkStreamTracer streamer
    streamer SetInputConnection [reader GetOutputPort]
    streamer SetStartPosition 0.1 2.1 0.5
    streamer SetMaximumPropagation 500
    streamer SetInitialIntegrationStep 0.05
    streamer SetIntegrationDirectionToBoth
    streamer SetIntegrator integ

# The tube is wrapped around the generated streamline. By varying the radius
# by the inverse of vector magnitude, we are creating a tube whose radius is
# proportional to mass flux (in incompressible flow).
vtkTubeFilter streamTube
    streamTube SetInputConnection [streamer GetOutputPort]
    streamTube SetInputArrayToProcess 1 0 0 vtkDataObject::FIELD_ASSOCIATION_POINTS vectors
    streamTube SetRadius 0.02
    streamTube SetNumberOfSides 12
    streamTube SetVaryRadiusToVaryRadiusByVector
vtkPolyDataMapper mapStreamTube
    mapStreamTube SetInputConnection [streamTube GetOutputPort]
    eval mapStreamTube SetScalarRange \
       [[[[reader GetOutput] GetPointData] GetScalars] GetRange]
vtkActor streamTubeActor
    streamTubeActor SetMapper mapStreamTube
    [streamTubeActor GetProperty] BackfaceCullingOn

# From here on we generate a whole bunch of planes which correspond to
# the geometry in the analysis; tables, bookshelves and so on.
vtkStructuredGridGeometryFilter table1
    table1 SetInputConnection [reader GetOutputPort]
    table1 SetExtent 11 15 7 9 8 8
vtkPolyDataMapper mapTable1
    mapTable1 SetInputConnection [table1 GetOutputPort]
    mapTable1 ScalarVisibilityOff
vtkActor table1Actor
    table1Actor SetMapper mapTable1
    [table1Actor GetProperty] SetColor .59 .427 .392

vtkStructuredGridGeometryFilter table2
    table2 SetInputConnection [reader GetOutputPort]
    table2 SetExtent 11 15 10 12 8 8
vtkPolyDataMapper mapTable2
    mapTable2 SetInputConnection [table2 GetOutputPort]
    mapTable2 ScalarVisibilityOff
vtkActor table2Actor
    table2Actor SetMapper mapTable2
    [table2Actor GetProperty] SetColor .59 .427 .392

vtkStructuredGridGeometryFilter FilingCabinet1
    FilingCabinet1 SetInputConnection [reader GetOutputPort]
    FilingCabinet1 SetExtent 15 15 7 9 0 8
vtkPolyDataMapper mapFilingCabinet1
    mapFilingCabinet1 SetInputConnection [FilingCabinet1 GetOutputPort]
    mapFilingCabinet1 ScalarVisibilityOff
vtkActor FilingCabinet1Actor
    FilingCabinet1Actor SetMapper mapFilingCabinet1
    [FilingCabinet1Actor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter FilingCabinet2
    FilingCabinet2 SetInputConnection [reader GetOutputPort]
    FilingCabinet2 SetExtent 15 15 10 12 0 8
vtkPolyDataMapper mapFilingCabinet2
    mapFilingCabinet2 SetInputConnection [FilingCabinet2 GetOutputPort]
    mapFilingCabinet2 ScalarVisibilityOff
vtkActor FilingCabinet2Actor
    FilingCabinet2Actor SetMapper mapFilingCabinet2
    [FilingCabinet2Actor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Top
    bookshelf1Top SetInputConnection [reader GetOutputPort]
    bookshelf1Top SetExtent 13 13 0 4 0 11
vtkPolyDataMapper mapBookshelf1Top
    mapBookshelf1Top SetInputConnection [bookshelf1Top GetOutputPort]
    mapBookshelf1Top ScalarVisibilityOff
vtkActor bookshelf1TopActor
    bookshelf1TopActor SetMapper mapBookshelf1Top
    [bookshelf1TopActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Bottom
    bookshelf1Bottom SetInputConnection [reader GetOutputPort]
    bookshelf1Bottom SetExtent 20 20 0 4 0 11
vtkPolyDataMapper mapBookshelf1Bottom
    mapBookshelf1Bottom SetInputConnection [bookshelf1Bottom GetOutputPort]
    mapBookshelf1Bottom ScalarVisibilityOff
vtkActor bookshelf1BottomActor
    bookshelf1BottomActor SetMapper mapBookshelf1Bottom
    [bookshelf1BottomActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Front
    bookshelf1Front SetInputConnection [reader GetOutputPort]
    bookshelf1Front SetExtent 13 20 0 0 0 11
vtkPolyDataMapper mapBookshelf1Front
    mapBookshelf1Front SetInputConnection [bookshelf1Front GetOutputPort]
    mapBookshelf1Front ScalarVisibilityOff
vtkActor bookshelf1FrontActor
    bookshelf1FrontActor SetMapper mapBookshelf1Front
    [bookshelf1FrontActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1Back
    bookshelf1Back SetInputConnection [reader GetOutputPort]
    bookshelf1Back SetExtent 13 20 4 4 0 11
vtkPolyDataMapper mapBookshelf1Back
    mapBookshelf1Back SetInputConnection [bookshelf1Back GetOutputPort]
    mapBookshelf1Back ScalarVisibilityOff
vtkActor bookshelf1BackActor
    bookshelf1BackActor SetMapper mapBookshelf1Back
    [bookshelf1BackActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1LHS
    bookshelf1LHS SetInputConnection [reader GetOutputPort]
    bookshelf1LHS SetExtent 13 20 0 4 0 0
vtkPolyDataMapper mapBookshelf1LHS
    mapBookshelf1LHS SetInputConnection [bookshelf1LHS GetOutputPort]
    mapBookshelf1LHS ScalarVisibilityOff
vtkActor bookshelf1LHSActor
    bookshelf1LHSActor SetMapper mapBookshelf1LHS
    [bookshelf1LHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf1RHS
    bookshelf1RHS SetInputConnection [reader GetOutputPort]
    bookshelf1RHS SetExtent 13 20 0 4 11 11
vtkPolyDataMapper mapBookshelf1RHS
    mapBookshelf1RHS SetInputConnection [bookshelf1RHS GetOutputPort]
    mapBookshelf1RHS ScalarVisibilityOff
vtkActor bookshelf1RHSActor
    bookshelf1RHSActor SetMapper mapBookshelf1RHS
    [bookshelf1RHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Top
    bookshelf2Top SetInputConnection [reader GetOutputPort]
    bookshelf2Top SetExtent 13 13 15 19 0 11
vtkPolyDataMapper mapBookshelf2Top
    mapBookshelf2Top SetInputConnection [bookshelf2Top GetOutputPort]
    mapBookshelf2Top ScalarVisibilityOff
vtkActor bookshelf2TopActor
    bookshelf2TopActor SetMapper mapBookshelf2Top
    [bookshelf2TopActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Bottom
    bookshelf2Bottom SetInputConnection [reader GetOutputPort]
    bookshelf2Bottom SetExtent 20 20 15 19 0 11
vtkPolyDataMapper mapBookshelf2Bottom
    mapBookshelf2Bottom SetInputConnection [bookshelf2Bottom GetOutputPort]
    mapBookshelf2Bottom ScalarVisibilityOff
vtkActor bookshelf2BottomActor
    bookshelf2BottomActor SetMapper mapBookshelf2Bottom
    [bookshelf2BottomActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Front
    bookshelf2Front SetInputConnection [reader GetOutputPort]
    bookshelf2Front SetExtent 13 20 15 15 0 11
vtkPolyDataMapper mapBookshelf2Front
    mapBookshelf2Front SetInputConnection [bookshelf2Front GetOutputPort]
    mapBookshelf2Front ScalarVisibilityOff
vtkActor bookshelf2FrontActor
    bookshelf2FrontActor SetMapper mapBookshelf2Front
    [bookshelf2FrontActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2Back
    bookshelf2Back SetInputConnection [reader GetOutputPort]
    bookshelf2Back SetExtent 13 20 19 19 0 11
vtkPolyDataMapper mapBookshelf2Back
    mapBookshelf2Back SetInputConnection [bookshelf2Back GetOutputPort]
    mapBookshelf2Back ScalarVisibilityOff
vtkActor bookshelf2BackActor
    bookshelf2BackActor SetMapper mapBookshelf2Back
    [bookshelf2BackActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2LHS
    bookshelf2LHS SetInputConnection [reader GetOutputPort]
    bookshelf2LHS SetExtent 13 20 15 19 0 0
vtkPolyDataMapper mapBookshelf2LHS
    mapBookshelf2LHS SetInputConnection [bookshelf2LHS GetOutputPort]
    mapBookshelf2LHS ScalarVisibilityOff
vtkActor bookshelf2LHSActor
    bookshelf2LHSActor SetMapper mapBookshelf2LHS
    [bookshelf2LHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter bookshelf2RHS
    bookshelf2RHS SetInputConnection [reader GetOutputPort]
    bookshelf2RHS SetExtent 13 20 15 19 11 11
vtkPolyDataMapper mapBookshelf2RHS
    mapBookshelf2RHS SetInputConnection [bookshelf2RHS GetOutputPort]
    mapBookshelf2RHS ScalarVisibilityOff
vtkActor bookshelf2RHSActor
    bookshelf2RHSActor SetMapper mapBookshelf2RHS
    [bookshelf2RHSActor GetProperty] SetColor .8 .8 .6

vtkStructuredGridGeometryFilter window
    window SetInputConnection [reader GetOutputPort]
    window SetExtent 20 20 6 13 10 13
vtkPolyDataMapper mapWindow
    mapWindow SetInputConnection [window GetOutputPort]
    mapWindow ScalarVisibilityOff
vtkActor windowActor
    windowActor SetMapper mapWindow
    [windowActor GetProperty] SetColor .3 .3 .5

vtkStructuredGridGeometryFilter outlet
    outlet SetInputConnection [reader GetOutputPort]
    outlet SetExtent 0 0 9 10 14 16
vtkPolyDataMapper mapOutlet
    mapOutlet SetInputConnection [outlet GetOutputPort]
    mapOutlet ScalarVisibilityOff
vtkActor outletActor
    outletActor SetMapper mapOutlet
    [outletActor GetProperty] SetColor 0 0 0

vtkStructuredGridGeometryFilter inlet
    inlet SetInputConnection [reader GetOutputPort]
    inlet SetExtent 0 0 9 10 0 6
vtkPolyDataMapper mapInlet
    mapInlet SetInputConnection [inlet GetOutputPort]
    mapInlet ScalarVisibilityOff
vtkActor inletActor
    inletActor SetMapper mapInlet
    [inletActor GetProperty] SetColor 0 0 0

vtkStructuredGridOutlineFilter outline
    outline SetInputConnection [reader GetOutputPort]
vtkPolyDataMapper mapOutline
    mapOutline SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper mapOutline
    [outlineActor GetProperty] SetColor 0 0 0

# Now create the usual graphics stuff.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

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
ren1 AddActor streamTubeActor

eval ren1 SetBackground $slate_grey

# Here we specify a particular view.
vtkCamera aCamera
    aCamera SetClippingRange 0.726079 36.3039
    aCamera SetFocalPoint 2.43584 2.15046 1.11104
    aCamera SetPosition -4.76183 -10.4426 3.17203
    aCamera SetViewUp 0.0511273 0.132773 0.989827
    aCamera SetViewAngle 18.604;
    aCamera Zoom 1.2

ren1 SetActiveCamera aCamera

renWin SetSize 500 300
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# interact with data
wm withdraw .
