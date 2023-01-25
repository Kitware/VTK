#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersGeneral import (
    vtkClipDataSet,
    vtkDataSetTriangleFilter,
    vtkShrinkFilter,
)
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
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

# read the football dataset:
#
reader = vtkUnstructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/PentaHexa.vtk")
reader.Update()
# Clip
#
plane = vtkPlane()
plane.SetNormal(1,1,0)
clip = vtkClipDataSet()
clip.SetInputConnection(reader.GetOutputPort())
clip.SetClipFunction(plane)
clip.GenerateClipScalarsOn()
g = vtkDataSetSurfaceFilter()
g.SetInputConnection(clip.GetOutputPort())
map = vtkPolyDataMapper()
map.SetInputConnection(g.GetOutputPort())
clipActor = vtkActor()
clipActor.SetMapper(map)
# Contour
#
contour = vtkContourFilter()
contour.SetInputConnection(reader.GetOutputPort())
contour.SetValue(0,0.125)
contour.SetValue(1,0.25)
contour.SetValue(2,0.5)
contour.SetValue(3,0.75)
contour.SetValue(4,1.0)
g2 = vtkDataSetSurfaceFilter()
g2.SetInputConnection(contour.GetOutputPort())
map2 = vtkPolyDataMapper()
map2.SetInputConnection(g2.GetOutputPort())
map2.ScalarVisibilityOff()
contourActor = vtkActor()
contourActor.SetMapper(map2)
contourActor.GetProperty().SetColor(1,0,0)
contourActor.GetProperty().SetRepresentationToWireframe()
# Triangulate
tris = vtkDataSetTriangleFilter()
tris.SetInputConnection(reader.GetOutputPort())
shrink = vtkShrinkFilter()
shrink.SetInputConnection(tris.GetOutputPort())
shrink.SetShrinkFactor(.8)
map3 = vtkDataSetMapper()
map3.SetInputConnection(shrink.GetOutputPort())
map3.SetScalarRange(0,26)
triActor = vtkActor()
triActor.SetMapper(map3)
triActor.AddPosition(2,0,0)
# Create graphics stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(contourActor)
ren1.AddActor(triActor)
ren1.SetBackground(1,1,1)
renWin.Render()
# render the image
#
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
