#!/usr/bin/env python

# This example demonstrates the use of streamlines generated from seeds, 
# combined with a tube filter to create several streamtubes.

import vtk
from vtk.util.misc import vtkGetDataRoot
from vtk.util.colors import *
VTK_DATA_ROOT = vtkGetDataRoot()

# We read a data file the is a CFD analysis of airflow in an office
# (with ventilation and a burning cigarette). We force an update so
# that we can query the output for its length, i.e., the length of the
# diagonal of the bounding box. This is useful for normalizing the
# data.
reader = vtk.vtkStructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/office.binary.vtk")
reader.Update()

length = reader.GetOutput().GetLength()

maxVelocity =reader.GetOutput().GetPointData().GetVectors().GetMaxNorm()
maxTime = 35.0*length/maxVelocity

# Now we will generate multiple streamlines in the data. We create a
# random cloud of points and then use those as integration seeds. We
# select the integration order to use (RungeKutta order 4) and
# associate it with the streamer. The start position is the position
# in world space where we want to begin streamline integration; and we
# integrate in both directions. The step length is the length of the
# line segments that make up the streamline (i.e., related to
# display). The IntegrationStepLength specifies the integration step
# length as a fraction of the cell size that the streamline is in.

# Create source for streamtubes
seeds = vtk.vtkPointSource()
seeds.SetRadius(0.15)
seeds.SetCenter(0.1, 2.1, 0.5)
seeds.SetNumberOfPoints(6)

integ = vtk.vtkRungeKutta4()
streamer = vtk.vtkStreamLine()
streamer.SetInputConnection(reader.GetOutputPort())
streamer.SetSource(seeds.GetOutput())
streamer.SetMaximumPropagationTime(500)
streamer.SetStepLength(0.5)
streamer.SetIntegrationStepLength(0.05)
streamer.SetIntegrationDirectionToIntegrateBothDirections()
streamer.SetIntegrator(integ)

# The tube is wrapped around the generated streamline. By varying the
# radius by the inverse of vector magnitude, we are creating a tube
# whose radius is proportional to mass flux (in incompressible flow).
streamTube = vtk.vtkTubeFilter()
streamTube.SetInputConnection(streamer.GetOutputPort())
streamTube.SetRadius(0.02)
streamTube.SetNumberOfSides(12)
streamTube.SetVaryRadiusToVaryRadiusByVector()
mapStreamTube = vtk.vtkPolyDataMapper()
mapStreamTube.SetInputConnection(streamTube.GetOutputPort())
mapStreamTube.SetScalarRange(reader.GetOutput().GetPointData().GetScalars().GetRange())
streamTubeActor = vtk.vtkActor()
streamTubeActor.SetMapper(mapStreamTube)
streamTubeActor.GetProperty().BackfaceCullingOn()

# From here on we generate a whole bunch of planes which correspond to
# the geometry in the analysis; tables, bookshelves and so on.
table1 = vtk.vtkStructuredGridGeometryFilter()
table1.SetInputConnection(reader.GetOutputPort())
table1.SetExtent(11, 15, 7, 9, 8, 8)
mapTable1 = vtk.vtkPolyDataMapper()
mapTable1.SetInputConnection(table1.GetOutputPort())
mapTable1.ScalarVisibilityOff()
table1Actor = vtk.vtkActor()
table1Actor.SetMapper(mapTable1)
table1Actor.GetProperty().SetColor(.59, .427, .392)

table2 = vtk.vtkStructuredGridGeometryFilter()
table2.SetInputConnection(reader.GetOutputPort())
table2.SetExtent(11, 15, 10, 12, 8, 8)
mapTable2 = vtk.vtkPolyDataMapper()
mapTable2.SetInputConnection(table2.GetOutputPort())
mapTable2.ScalarVisibilityOff()
table2Actor = vtk.vtkActor()
table2Actor.SetMapper(mapTable2)
table2Actor.GetProperty().SetColor(.59, .427, .392)

FilingCabinet1 = vtk.vtkStructuredGridGeometryFilter()
FilingCabinet1.SetInputConnection(reader.GetOutputPort())
FilingCabinet1.SetExtent(15, 15, 7, 9, 0, 8)
mapFilingCabinet1 = vtk.vtkPolyDataMapper()
mapFilingCabinet1.SetInputConnection(FilingCabinet1.GetOutputPort())
mapFilingCabinet1.ScalarVisibilityOff()
FilingCabinet1Actor = vtk.vtkActor()
FilingCabinet1Actor.SetMapper(mapFilingCabinet1)
FilingCabinet1Actor.GetProperty().SetColor(.8, .8, .6)

FilingCabinet2 = vtk.vtkStructuredGridGeometryFilter()
FilingCabinet2.SetInputConnection(reader.GetOutputPort())
FilingCabinet2.SetExtent(15, 15, 10, 12, 0, 8)
mapFilingCabinet2 = vtk.vtkPolyDataMapper()
mapFilingCabinet2.SetInputConnection(FilingCabinet2.GetOutputPort())
mapFilingCabinet2.ScalarVisibilityOff()
FilingCabinet2Actor = vtk.vtkActor()
FilingCabinet2Actor.SetMapper(mapFilingCabinet2)
FilingCabinet2Actor.GetProperty().SetColor(.8, .8, .6)

bookshelf1Top = vtk.vtkStructuredGridGeometryFilter()
bookshelf1Top.SetInputConnection(reader.GetOutputPort())
bookshelf1Top.SetExtent(13, 13, 0, 4, 0, 11)
mapBookshelf1Top = vtk.vtkPolyDataMapper()
mapBookshelf1Top.SetInputConnection(bookshelf1Top.GetOutputPort())
mapBookshelf1Top.ScalarVisibilityOff()
bookshelf1TopActor = vtk.vtkActor()
bookshelf1TopActor.SetMapper(mapBookshelf1Top)
bookshelf1TopActor.GetProperty().SetColor(.8, .8, .6)

bookshelf1Bottom = vtk.vtkStructuredGridGeometryFilter()
bookshelf1Bottom.SetInputConnection(reader.GetOutputPort())
bookshelf1Bottom.SetExtent(20, 20, 0, 4, 0, 11)
mapBookshelf1Bottom = vtk.vtkPolyDataMapper()
mapBookshelf1Bottom.SetInputConnection(bookshelf1Bottom.GetOutputPort())
mapBookshelf1Bottom.ScalarVisibilityOff()
bookshelf1BottomActor = vtk.vtkActor()
bookshelf1BottomActor.SetMapper(mapBookshelf1Bottom)
bookshelf1BottomActor.GetProperty().SetColor(.8, .8, .6)

bookshelf1Front = vtk.vtkStructuredGridGeometryFilter()
bookshelf1Front.SetInputConnection(reader.GetOutputPort())
bookshelf1Front.SetExtent(13, 20, 0, 0, 0, 11)
mapBookshelf1Front = vtk.vtkPolyDataMapper()
mapBookshelf1Front.SetInputConnection(bookshelf1Front.GetOutputPort())
mapBookshelf1Front.ScalarVisibilityOff()
bookshelf1FrontActor = vtk.vtkActor()
bookshelf1FrontActor.SetMapper(mapBookshelf1Front)
bookshelf1FrontActor.GetProperty().SetColor(.8, .8, .6)

bookshelf1Back = vtk.vtkStructuredGridGeometryFilter()
bookshelf1Back.SetInputConnection(reader.GetOutputPort())
bookshelf1Back.SetExtent(13, 20, 4, 4, 0, 11)
mapBookshelf1Back = vtk.vtkPolyDataMapper()
mapBookshelf1Back.SetInputConnection(bookshelf1Back.GetOutputPort())
mapBookshelf1Back.ScalarVisibilityOff()
bookshelf1BackActor = vtk.vtkActor()
bookshelf1BackActor.SetMapper(mapBookshelf1Back)
bookshelf1BackActor.GetProperty().SetColor(.8, .8, .6)

bookshelf1LHS = vtk.vtkStructuredGridGeometryFilter()
bookshelf1LHS.SetInputConnection(reader.GetOutputPort())
bookshelf1LHS.SetExtent(13, 20, 0, 4, 0, 0)
mapBookshelf1LHS = vtk.vtkPolyDataMapper()
mapBookshelf1LHS.SetInputConnection(bookshelf1LHS.GetOutputPort())
mapBookshelf1LHS.ScalarVisibilityOff()
bookshelf1LHSActor = vtk.vtkActor()
bookshelf1LHSActor.SetMapper(mapBookshelf1LHS)
bookshelf1LHSActor.GetProperty().SetColor(.8, .8, .6)

bookshelf1RHS = vtk.vtkStructuredGridGeometryFilter()
bookshelf1RHS.SetInputConnection(reader.GetOutputPort())
bookshelf1RHS.SetExtent(13, 20, 0, 4, 11, 11)
mapBookshelf1RHS = vtk.vtkPolyDataMapper()
mapBookshelf1RHS.SetInputConnection(bookshelf1RHS.GetOutputPort())
mapBookshelf1RHS.ScalarVisibilityOff()
bookshelf1RHSActor = vtk.vtkActor()
bookshelf1RHSActor.SetMapper(mapBookshelf1RHS)
bookshelf1RHSActor.GetProperty().SetColor(.8, .8, .6)

bookshelf2Top = vtk.vtkStructuredGridGeometryFilter()
bookshelf2Top.SetInputConnection(reader.GetOutputPort())
bookshelf2Top.SetExtent(13, 13, 15, 19, 0, 11)
mapBookshelf2Top = vtk.vtkPolyDataMapper()
mapBookshelf2Top.SetInputConnection(bookshelf2Top.GetOutputPort())
mapBookshelf2Top.ScalarVisibilityOff()
bookshelf2TopActor = vtk.vtkActor()
bookshelf2TopActor.SetMapper(mapBookshelf2Top)
bookshelf2TopActor.GetProperty().SetColor(.8, .8, .6)

bookshelf2Bottom = vtk.vtkStructuredGridGeometryFilter()
bookshelf2Bottom.SetInputConnection(reader.GetOutputPort())
bookshelf2Bottom.SetExtent(20, 20, 15, 19, 0, 11)
mapBookshelf2Bottom = vtk.vtkPolyDataMapper()
mapBookshelf2Bottom.SetInputConnection(bookshelf2Bottom.GetOutputPort())
mapBookshelf2Bottom.ScalarVisibilityOff()
bookshelf2BottomActor = vtk.vtkActor()
bookshelf2BottomActor.SetMapper(mapBookshelf2Bottom)
bookshelf2BottomActor.GetProperty().SetColor(.8, .8, .6)

bookshelf2Front = vtk.vtkStructuredGridGeometryFilter()
bookshelf2Front.SetInputConnection(reader.GetOutputPort())
bookshelf2Front.SetExtent(13, 20, 15, 15, 0, 11)
mapBookshelf2Front = vtk.vtkPolyDataMapper()
mapBookshelf2Front.SetInputConnection(bookshelf2Front.GetOutputPort())
mapBookshelf2Front.ScalarVisibilityOff()
bookshelf2FrontActor = vtk.vtkActor()
bookshelf2FrontActor.SetMapper(mapBookshelf2Front)
bookshelf2FrontActor.GetProperty().SetColor(.8, .8, .6)

bookshelf2Back = vtk.vtkStructuredGridGeometryFilter()
bookshelf2Back.SetInputConnection(reader.GetOutputPort())
bookshelf2Back.SetExtent(13, 20, 19, 19, 0, 11)
mapBookshelf2Back = vtk.vtkPolyDataMapper()
mapBookshelf2Back.SetInputConnection(bookshelf2Back.GetOutputPort())
mapBookshelf2Back.ScalarVisibilityOff()
bookshelf2BackActor = vtk.vtkActor()
bookshelf2BackActor.SetMapper(mapBookshelf2Back)
bookshelf2BackActor.GetProperty().SetColor(.8, .8, .6)

bookshelf2LHS = vtk.vtkStructuredGridGeometryFilter()
bookshelf2LHS.SetInputConnection(reader.GetOutputPort())
bookshelf2LHS.SetExtent(13, 20, 15, 19, 0, 0)
mapBookshelf2LHS = vtk.vtkPolyDataMapper()
mapBookshelf2LHS.SetInputConnection(bookshelf2LHS.GetOutputPort())
mapBookshelf2LHS.ScalarVisibilityOff()
bookshelf2LHSActor = vtk.vtkActor()
bookshelf2LHSActor.SetMapper(mapBookshelf2LHS)
bookshelf2LHSActor.GetProperty().SetColor(.8, .8, .6)

bookshelf2RHS = vtk.vtkStructuredGridGeometryFilter()
bookshelf2RHS.SetInputConnection(reader.GetOutputPort())
bookshelf2RHS.SetExtent(13, 20, 15, 19, 11, 11)
mapBookshelf2RHS = vtk.vtkPolyDataMapper()
mapBookshelf2RHS.SetInputConnection(bookshelf2RHS.GetOutputPort())
mapBookshelf2RHS.ScalarVisibilityOff()
bookshelf2RHSActor = vtk.vtkActor()
bookshelf2RHSActor.SetMapper(mapBookshelf2RHS)
bookshelf2RHSActor.GetProperty().SetColor(.8, .8, .6)

window = vtk.vtkStructuredGridGeometryFilter()
window.SetInputConnection(reader.GetOutputPort())
window.SetExtent(20, 20, 6, 13, 10, 13)
mapWindow = vtk.vtkPolyDataMapper()
mapWindow.SetInputConnection(window.GetOutputPort())
mapWindow.ScalarVisibilityOff()
windowActor = vtk.vtkActor()
windowActor.SetMapper(mapWindow)
windowActor.GetProperty().SetColor(.3, .3, .5)

outlet = vtk.vtkStructuredGridGeometryFilter()
outlet.SetInputConnection(reader.GetOutputPort())
outlet.SetExtent(0, 0, 9, 10, 14, 16)
mapOutlet = vtk.vtkPolyDataMapper()
mapOutlet.SetInputConnection(outlet.GetOutputPort())
mapOutlet.ScalarVisibilityOff()
outletActor = vtk.vtkActor()
outletActor.SetMapper(mapOutlet)
outletActor.GetProperty().SetColor(0, 0, 0)

inlet = vtk.vtkStructuredGridGeometryFilter()
inlet.SetInputConnection(reader.GetOutputPort())
inlet.SetExtent(0, 0, 9, 10, 0, 6)
mapInlet = vtk.vtkPolyDataMapper()
mapInlet.SetInputConnection(inlet.GetOutputPort())
mapInlet.ScalarVisibilityOff()
inletActor = vtk.vtkActor()
inletActor.SetMapper(mapInlet)
inletActor.GetProperty().SetColor(0, 0, 0)

outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())
mapOutline = vtk.vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Now create the usual graphics stuff.
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(table1Actor)
ren.AddActor(table2Actor)
ren.AddActor(FilingCabinet1Actor)
ren.AddActor(FilingCabinet2Actor)
ren.AddActor(bookshelf1TopActor)
ren.AddActor(bookshelf1BottomActor)
ren.AddActor(bookshelf1FrontActor)
ren.AddActor(bookshelf1BackActor)
ren.AddActor(bookshelf1LHSActor)
ren.AddActor(bookshelf1RHSActor)
ren.AddActor(bookshelf2TopActor)
ren.AddActor(bookshelf2BottomActor)
ren.AddActor(bookshelf2FrontActor)
ren.AddActor(bookshelf2BackActor)
ren.AddActor(bookshelf2LHSActor)
ren.AddActor(bookshelf2RHSActor)
ren.AddActor(windowActor)
ren.AddActor(outletActor)
ren.AddActor(inletActor)
ren.AddActor(outlineActor)
ren.AddActor(streamTubeActor)

ren.SetBackground(slate_grey)

# Here we specify a particular view.
aCamera = vtk.vtkCamera()
aCamera.SetClippingRange(0.726079, 36.3039)
aCamera.SetFocalPoint(2.43584, 2.15046, 1.11104)
aCamera.SetPosition(-4.76183, -10.4426, 3.17203)
aCamera.SetViewUp(0.0511273, 0.132773, 0.989827)
aCamera.SetViewAngle(18.604)
aCamera.Zoom(1.2)

ren.SetActiveCamera(aCamera)
renWin.SetSize(500, 300)

iren.Initialize()
renWin.Render()
iren.Start()
