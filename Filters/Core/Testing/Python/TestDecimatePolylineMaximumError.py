#!/usr/bin/env python

# Decimate polyline with maximum error
from vtkmodules.vtkFiltersCore import (
    vtkDecimatePolylineFilter,
    vtkFeatureEdges,
    vtkStripper,
)
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
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create rendering stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);

# Plane source is used to create a polyline around boundary and then
# decimated.
ps = vtkPlaneSource()
ps.SetResolution(50,100)

fe = vtkFeatureEdges()
fe.SetInputConnection(ps.GetOutputPort())
fe.BoundaryEdgesOn()
fe.FeatureEdgesOff()
fe.NonManifoldEdgesOff()

s = vtkStripper()
s.SetInputConnection(fe.GetOutputPort())

deci = vtkDecimatePolylineFilter()
deci.SetInputConnection(s.GetOutputPort())
deci.SetMaximumError(0.00001)
deci.SetTargetReduction(0.99)
deci.Update()

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(deci.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(actor)

cam = ren.GetActiveCamera()
cam.SetPosition(0,0,1)
cam.SetFocalPoint(0,0,0)
ren.ResetCamera()

# render and interact with data
renWin.SetSize(300, 300)
renWin.Render()
iRen.Start()
