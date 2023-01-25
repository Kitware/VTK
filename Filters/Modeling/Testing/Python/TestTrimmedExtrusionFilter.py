#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkLookupTable,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import vtkTriangleFilter
from vtkmodules.vtkFiltersGeneral import vtkWarpScalar
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkTrimmedExtrusionFilter
from vtkmodules.vtkIOImage import vtkDEMReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCellPicker,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline. Load terrain data.
#
lut = vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

# Read the data: a height field results
demReader = vtkDEMReader()
demReader.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demReader.Update()

lo = demReader.GetOutput().GetScalarRange()[0]
hi = demReader.GetOutput().GetScalarRange()[1]
bds = demReader.GetOutput().GetBounds()
#print("Bounds: {0}".format(bds))

surface = vtkImageDataGeometryFilter()
surface.SetInputConnection(demReader.GetOutputPort())

tris = vtkTriangleFilter()
tris.SetInputConnection(surface.GetOutputPort())

warp = vtkWarpScalar()
warp.SetInputConnection(tris.GetOutputPort())
warp.SetScaleFactor(1)
warp.UseNormalOn()
warp.SetNormal(0, 0, 1)
warp.Update()

# Show the terrain
demMapper = vtkPolyDataMapper()
demMapper.SetInputConnection(warp.GetOutputPort())
demMapper.SetScalarRange(lo, hi)
demMapper.SetLookupTable(lut)

demActor = vtkActor()
demActor.SetMapper(demMapper)

# Create polygon(s) to extrude
polygons = vtkPolyData()

pts = vtkPoints()
pts.SetNumberOfPoints(14)
pts.SetPoint(0, 560000, 5110000, 2000)
pts.SetPoint(1, 560250, 5110000, 2000)
pts.SetPoint(2, 560250, 5110250, 2000)
pts.SetPoint(3, 560000, 5110250, 2000)

pts.SetPoint(4, 560500, 5110000, 2100)
pts.SetPoint(5, 560750, 5110000, 2100)
pts.SetPoint(6, 560750, 5110250, 2100)
pts.SetPoint(7, 560500, 5110250, 2100)

pts.SetPoint(8, 559800, 5110500, 1950)
pts.SetPoint(9, 560500, 5110500, 1950)
pts.SetPoint(10, 560500, 5110800, 1950)
pts.SetPoint(11, 560150, 5110800, 1950)
pts.SetPoint(12, 560150, 5111100, 1950)
pts.SetPoint(13, 559800, 5111100, 1950)

polys = vtkCellArray()
polys.InsertNextCell(4)
polys.InsertCellPoint(0)
polys.InsertCellPoint(1)
polys.InsertCellPoint(2)
polys.InsertCellPoint(3)
polys.InsertNextCell(4)
polys.InsertCellPoint(4)
polys.InsertCellPoint(5)
polys.InsertCellPoint(6)
polys.InsertCellPoint(7)
polys.InsertNextCell(6)
polys.InsertCellPoint(8)
polys.InsertCellPoint(9)
polys.InsertCellPoint(10)
polys.InsertCellPoint(11)
polys.InsertCellPoint(12)
polys.InsertCellPoint(13)

polygons.SetPoints(pts)
polygons.SetPolys(polys)

# Extrude polygon down to surface
extrude = vtkTrimmedExtrusionFilter()
extrude.SetInputData(polygons)
extrude.SetTrimSurfaceConnection(warp.GetOutputPort())
extrude.SetExtrusionDirection(0,0,1)

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(extrude.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = vtkActor()
actor.SetMapper(mapper)

# Show generating polygons
polyMapper = vtkPolyDataMapper()
polyMapper.SetInputData(polygons)
polyMapper.ScalarVisibilityOff()

# Offset slightly to avoid zbuffer issues
polyActor = vtkActor()
polyActor.SetMapper(polyMapper)
polyActor.GetProperty().SetColor(1,0,0)
polyActor.AddPosition(0,0,10)

# Render it
ren.AddActor(demActor)
ren.AddActor(actor)
ren.AddActor(polyActor)

ren.GetActiveCamera().SetPosition( 560752, 5110002, 2110)
ren.GetActiveCamera().SetFocalPoint( 560750, 5110000, 2100)
ren.ResetCamera()
ren.GetActiveCamera().SetClippingRange(269.775, 34560.4)
ren.GetActiveCamera().SetFocalPoint(562026, 5.1135e+006, -400.794)
ren.GetActiveCamera().SetPosition(556898, 5.10151e+006, 7906.19)

# added these unused default arguments so that the prototype
# matches as required in python.
def reportCamera(obj=None, event=""):
    print("Camera: {}".format(ren.GetActiveCamera()))

picker = vtkCellPicker()
picker.AddObserver("EndPickEvent",reportCamera)
iren.SetPicker(picker)

renWin.Render()
iren.Start()
