#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkGlyph2D
from vtkmodules.vtkFiltersSources import (
    vtkGlyphSource2D,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor2D,
    vtkPolyDataMapper2D,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTransformCoordinateSystems,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# pipeline stuff
#
sphere = vtkSphereSource()
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(20)

xform = vtkTransformCoordinateSystems()
xform.SetInputConnection(sphere.GetOutputPort())
xform.SetInputCoordinateSystemToWorld()
xform.SetOutputCoordinateSystemToDisplay()
xform.SetViewport(ren1)

gs = vtkGlyphSource2D()
gs.SetGlyphTypeToCircle()
gs.SetScale(20)
gs.FilledOff()
gs.CrossOn()
gs.Update()

# Create a table of glyphs
glypher = vtkGlyph2D()
glypher.SetInputConnection(xform.GetOutputPort())
glypher.SetSourceData(0, gs.GetOutput())
glypher.SetScaleModeToDataScalingOff()

mapper = vtkPolyDataMapper2D()
mapper.SetInputConnection(glypher.GetOutputPort())

glyphActor = vtkActor2D()
glyphActor.SetMapper(mapper)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(glyphActor)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

iren.Initialize()
renWin.Render()

# render the image
#iren.Start()
