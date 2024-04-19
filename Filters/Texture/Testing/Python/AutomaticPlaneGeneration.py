#!/usr/bin/env python
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkFiltersTexture import vtkTextureMapToPlane
from vtkmodules.vtkIOImage import vtkPNMReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
aPlane = vtkPlaneSource()
aPlane.SetCenter(-100,-100,-100)
aPlane.SetOrigin(-100,-100,-100)
aPlane.SetPoint1(-90,-100,-100)
aPlane.SetPoint2(-100,-90,-100)
aPlane.SetNormal(0,-1,1)
imageIn = vtkPNMReader()
imageIn.SetFileName(VTK_DATA_ROOT + "/Data/earth.ppm")
texture = vtkTexture()
texture.SetInputConnection(imageIn.GetOutputPort())
texturePlane = vtkTextureMapToPlane()
texturePlane.SetInputConnection(aPlane.GetOutputPort())
texturePlane.AutomaticPlaneGenerationOn()
planeMapper = vtkPolyDataMapper()
planeMapper.SetInputConnection(texturePlane.GetOutputPort())
texturedPlane = vtkActor()
texturedPlane.SetMapper(planeMapper)
texturedPlane.SetTexture(texture)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(texturedPlane)
#ren1 SetBackground 1 1 1
renWin.SetSize(200,200)
renWin.Render()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
