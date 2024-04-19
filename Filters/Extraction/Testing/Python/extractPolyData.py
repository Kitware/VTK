#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkCylinder
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import vtkGlyph3D
from vtkmodules.vtkFiltersExtraction import vtkExtractPolyDataGeometry
from vtkmodules.vtkFiltersSources import vtkSphereSource
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

# Demonstrate how to extract polygonal cells with an implicit function
# get the interactor ui
# create a sphere source and actor
#
sphere = vtkSphereSource()
sphere.SetThetaResolution(8)
sphere.SetPhiResolution(16)
sphere.SetRadius(1.5)

# Extraction stuff
t = vtkTransform()
t.RotateX(90)
cylfunc = vtkCylinder()
cylfunc.SetRadius(0.5)
cylfunc.SetTransform(t)
extract = vtkExtractPolyDataGeometry()
extract.SetInputConnection(sphere.GetOutputPort())
extract.SetImplicitFunction(cylfunc)
extract.ExtractBoundaryCellsOn()
extract.PassPointsOn()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(extract.GetOutputPort())
sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)

# Extraction stuff - now cull points
extract2 = vtkExtractPolyDataGeometry()
extract2.SetInputConnection(sphere.GetOutputPort())
extract2.SetImplicitFunction(cylfunc)
extract2.ExtractBoundaryCellsOn()
extract2.PassPointsOff()
sphereMapper2 = vtkPolyDataMapper()
sphereMapper2.SetInputConnection(extract2.GetOutputPort())
sphereActor2 = vtkActor ()
sphereActor2.SetMapper(sphereMapper2)
sphereActor2.AddPosition(2.5, 0, 0)

# Put some glyphs on the points
glyphSphere = vtkSphereSource()
glyphSphere.SetRadius(0.05)
glyph = vtkGlyph3D()
glyph.SetInputConnection(extract.GetOutputPort())
glyph.SetSourceConnection(glyphSphere.GetOutputPort())
glyph.SetScaleModeToDataScalingOff()
glyphMapper = vtkPolyDataMapper()
glyphMapper.SetInputConnection(glyph.GetOutputPort())
glyphActor = vtkActor()
glyphActor.SetMapper(glyphMapper)

glyph2 = vtkGlyph3D()
glyph2.SetInputConnection(extract2.GetOutputPort())
glyph2.SetSourceConnection(glyphSphere.GetOutputPort())
glyph2.SetScaleModeToDataScalingOff()
glyphMapper2 = vtkPolyDataMapper()
glyphMapper2.SetInputConnection(glyph2.GetOutputPort())
glyphActor2 = vtkActor()
glyphActor2.SetMapper(glyphMapper2)
glyphActor2.AddPosition(2.5, 0, 0)


# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetWindowName("vtk - extractPolyData")
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(sphereActor)
ren1.AddActor(glyphActor)
ren1.AddActor(sphereActor2)
ren1.AddActor(glyphActor2)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
renWin.Render()
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
