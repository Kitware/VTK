#!/usr/bin/env python
from vtkmodules.vtkFiltersHybrid import vtkEarthSource
from vtkmodules.vtkFiltersSources import vtkTexturedSphereSource
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
tss = vtkTexturedSphereSource()
tss.SetThetaResolution(18)
tss.SetPhiResolution(9)
earthMapper = vtkPolyDataMapper()
earthMapper.SetInputConnection(tss.GetOutputPort())
earthActor = vtkActor()
earthActor.SetMapper(earthMapper)
# load in the texture map
#
atext = vtkTexture()
pnmReader = vtkPNMReader()
pnmReader.SetFileName(VTK_DATA_ROOT + "/Data/earth.ppm")
atext.SetInputConnection(pnmReader.GetOutputPort())
atext.InterpolateOn()
earthActor.SetTexture(atext)
# create a earth source and actor
#
es = vtkEarthSource()
es.SetRadius(0.501)
es.SetOnRatio(2)
earth2Mapper = vtkPolyDataMapper()
earth2Mapper.SetInputConnection(es.GetOutputPort())
earth2Actor = vtkActor()
earth2Actor.SetMapper(earth2Mapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(earthActor)
ren1.AddActor(earth2Actor)
ren1.SetBackground(0,0,0.1)
renWin.SetSize(300,300)
# render the image
#
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
