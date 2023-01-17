#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkLookupTable,
    vtkMath,
)
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkFiltersCore import vtkTriangleFilter
from vtkmodules.vtkFiltersGeneral import vtkWarpScalar
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkFitToHeightMapFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
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

# Debugging control
iDim = 5
jDim = 7
imgSize = iDim * jDim

# Create the RenderWindow, Renderer
#
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtkRenderWindow()
renWin.AddRenderer( ren0 )
renWin.AddRenderer( ren1 )

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline.
#
lut = vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

# Create an image with random heights on each pixel
image = vtkImageData()
image.SetDimensions(iDim,jDim,1)
image.SetSpacing(1,1,1)
image.SetOrigin(0,0,0)

math = vtkMath()
math.RandomSeed(31415)
heights = vtkFloatArray()
heights.SetNumberOfTuples(imgSize)
for i in range(0,imgSize):
    heights.SetTuple1(i,math.Random(0,1))

image.GetPointData().SetScalars(heights)

# Warp the image data
lo = image.GetScalarRange()[0]
hi = image.GetScalarRange()[1]

surface = vtkImageDataGeometryFilter()
surface.SetInputData(image)

tris = vtkTriangleFilter()
tris.SetInputConnection(surface.GetOutputPort())

warp = vtkWarpScalar()
warp.SetInputConnection(tris.GetOutputPort())
warp.SetScaleFactor(1)
warp.UseNormalOn()
warp.SetNormal(0, 0, 1)

# Show the terrain
imgMapper = vtkPolyDataMapper()
imgMapper.SetInputConnection(warp.GetOutputPort())
imgMapper.SetScalarRange(lo, hi)
imgMapper.SetLookupTable(lut)

imgActor = vtkActor()
imgActor.SetMapper(imgMapper)

# Polydata to drape
plane = vtkPlaneSource()
plane.SetOrigin(-0.1,-0.1,0)
plane.SetPoint1(iDim-1+0.1,-0.1,0)
plane.SetPoint2(-0.1,jDim-1+0.1,0)
plane.SetResolution(40,20)

# Fit polygons to surface (point strategy)
fit = vtkFitToHeightMapFilter()
fit.SetInputConnection(plane.GetOutputPort())
fit.SetHeightMapData(image)
fit.SetFittingStrategyToPointProjection()
fit.UseHeightMapOffsetOn()
fit.Update()
print(fit.GetOutput().GetBounds())

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(fit.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Fit polygons to surface (cell strategy)
fit2 = vtkFitToHeightMapFilter()
fit2.SetInputConnection(plane.GetOutputPort())
fit2.SetHeightMapData(image)
fit2.SetFittingStrategyToCellAverageHeight()
fit2.UseHeightMapOffsetOn()
fit2.Update()

mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(fit2.GetOutputPort())
mapper2.ScalarVisibilityOff()

actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.GetProperty().SetColor(1,0,0)

# Render it
ren0.AddActor(actor)
ren0.AddActor(imgActor)
ren1.AddActor(actor2)
ren1.AddActor(imgActor)

ren0.GetActiveCamera().SetPosition(1,1,1)
ren0.ResetCamera()
ren1.SetActiveCamera(ren0.GetActiveCamera())

renWin.Render()
iren.Start()
