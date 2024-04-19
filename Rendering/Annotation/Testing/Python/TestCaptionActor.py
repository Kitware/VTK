#!/usr/bin/env python
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingAnnotation import vtkCaptionActor2D
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
sphere = vtkSphereSource()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)
coneGlyph = vtkConeSource()
coneGlyph.SetResolution(6)
sphereGlyph = vtkSphereSource()
sphereGlyph.SetThetaResolution(12)
sphereGlyph.SetPhiResolution(6)
caption = vtkCaptionActor2D()
caption.SetCaption("This is the\nsouth pole")
caption.SetAttachmentPoint(0,0,-0.5)
caption.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
caption.GetPositionCoordinate().SetReferenceCoordinate(None)
caption.GetPositionCoordinate().SetValue(0.05,0.05)
caption.SetWidth(0.25)
caption.SetHeight(0.15)
caption.ThreeDimensionalLeaderOn()
caption.SetLeaderGlyphConnection(coneGlyph.GetOutputPort())
caption.SetMaximumLeaderGlyphSize(10)
caption.SetLeaderGlyphSize(0.025)
caption.GetProperty().SetColor(1,0,0)
tprop = caption.GetCaptionTextProperty()
tprop.SetColor(caption.GetProperty().GetColor())
caption2 = vtkCaptionActor2D()
caption2.SetCaption("Santa lives here")
caption2.GetProperty().SetColor(1,0,0)
caption2.SetAttachmentPoint(0,0,0.5)
caption2.SetHeight(0.05)
caption2.BorderOff()
caption2.SetPosition(25,10)
caption2.ThreeDimensionalLeaderOff()
caption2.SetLeaderGlyphConnection(coneGlyph.GetOutputPort())
caption2.SetWidth(0.35)
caption2.SetHeight(0.10)
caption2.SetMaximumLeaderGlyphSize(10)
caption2.SetLeaderGlyphSize(0.025)
tprop = caption2.GetCaptionTextProperty()
tprop.SetColor(caption2.GetProperty().GetColor())
ren1.AddActor2D(caption2)
ren1.AddActor2D(caption)
ren1.AddActor(sphereActor)
ren1.SetBackground(1,1,1)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetPosition(1,0,0)
ren1.GetActiveCamera().SetViewUp(0,0,1)
ren1.ResetCamera()
renWin.SetSize(250,250)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
