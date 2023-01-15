#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkPolyDataConnectivityFilter,
    vtkStructuredGridOutlineFilter,
)
from vtkmodules.vtkFiltersGeometry import vtkStructuredGridGeometryFilter
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
# planes to connect
plane1 = vtkStructuredGridGeometryFilter()
plane1.SetInputData(output)
plane1.SetExtent(20,20,0,100,0,100)
conn = vtkPolyDataConnectivityFilter()
conn.SetInputConnection(plane1.GetOutputPort())
conn.ScalarConnectivityOn()
conn.SetScalarRange(0.19,0.25)
conn.Update()
#conn.Print()
plane1Map = vtkPolyDataMapper()
plane1Map.SetInputConnection(conn.GetOutputPort())
plane1Map.SetScalarRange(output.GetScalarRange())
plane1Actor = vtkActor()
plane1Actor.SetMapper(plane1Map)
plane1Actor.GetProperty().SetOpacity(0.999)
# outline
outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()
outlineProp.SetColor(0,0,0)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(plane1Actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
cam1 = vtkCamera()
cam1.SetClippingRange(14.29,63.53)
cam1.SetFocalPoint(8.58522,1.58266,30.6486)
cam1.SetPosition(37.6808,-20.1298,35.4016)
cam1.SetViewAngle(30)
cam1.SetViewUp(-0.0566235,0.140504,0.98846)
ren1.SetActiveCamera(cam1)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
