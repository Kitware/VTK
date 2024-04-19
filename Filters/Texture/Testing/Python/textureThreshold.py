#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkStructuredGridOutlineFilter
from vtkmodules.vtkFiltersGeometry import vtkStructuredGridGeometryFilter
from vtkmodules.vtkFiltersTexture import vtkThresholdTextureCoords
from vtkmodules.vtkIOLegacy import vtkStructuredPointsReader
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkDataSetMapper,
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
# read data
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/bluntfinxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/bluntfinq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
# wall
#
wall = vtkStructuredGridGeometryFilter()
wall.SetInputData(output)
wall.SetExtent(0,100,0,0,0,100)
wallMap = vtkPolyDataMapper()
wallMap.SetInputConnection(wall.GetOutputPort())
wallMap.ScalarVisibilityOff()
wallActor = vtkActor()
wallActor.SetMapper(wallMap)
wallActor.GetProperty().SetColor(0.8,0.8,0.8)
# fin
#
fin = vtkStructuredGridGeometryFilter()
fin.SetInputData(output)
fin.SetExtent(0,100,0,100,0,0)
finMap = vtkPolyDataMapper()
finMap.SetInputConnection(fin.GetOutputPort())
finMap.ScalarVisibilityOff()
finActor = vtkActor()
finActor.SetMapper(finMap)
finActor.GetProperty().SetColor(0.8,0.8,0.8)
# planes to threshold
tmap = vtkStructuredPointsReader()
tmap.SetFileName(VTK_DATA_ROOT + "/Data/texThres2.vtk")
texture = vtkTexture()
texture.SetInputConnection(tmap.GetOutputPort())
texture.InterpolateOff()
texture.RepeatOff()
plane1 = vtkStructuredGridGeometryFilter()
plane1.SetInputData(output)
plane1.SetExtent(10,10,0,100,0,100)
thresh1 = vtkThresholdTextureCoords()
thresh1.SetInputConnection(plane1.GetOutputPort())
thresh1.ThresholdByUpper(1.5)
plane1Map = vtkDataSetMapper()
plane1Map.SetInputConnection(thresh1.GetOutputPort())
plane1Map.SetScalarRange(output.GetScalarRange())
plane1Actor = vtkActor()
plane1Actor.SetMapper(plane1Map)
plane1Actor.SetTexture(texture)
plane1Actor.GetProperty().SetOpacity(0.999)
plane2 = vtkStructuredGridGeometryFilter()
plane2.SetInputData(output)
plane2.SetExtent(30,30,0,100,0,100)
thresh2 = vtkThresholdTextureCoords()
thresh2.SetInputConnection(plane2.GetOutputPort())
thresh2.ThresholdByLower(1.5)
plane2Map = vtkDataSetMapper()
plane2Map.SetInputConnection(thresh2.GetOutputPort())
plane2Map.SetScalarRange(output.GetScalarRange())
plane2Actor = vtkActor()
plane2Actor.SetMapper(plane2Map)
plane2Actor.SetTexture(texture)
plane2Actor.GetProperty().SetOpacity(0.999)
plane3 = vtkStructuredGridGeometryFilter()
plane3.SetInputData(output)
plane3.SetExtent(35,35,0,100,0,100)
thresh3 = vtkThresholdTextureCoords()
thresh3.SetInputConnection(plane3.GetOutputPort())
thresh3.ThresholdBetween(1.5,1.8)
plane3Map = vtkDataSetMapper()
plane3Map.SetInputConnection(thresh3.GetOutputPort())
plane3Map.SetScalarRange(output.GetScalarRange())
plane3Actor = vtkActor()
plane3Actor.SetMapper(plane3Map)
plane3Actor.SetTexture(texture)
plane3Actor.GetProperty().SetOpacity(0.999)
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
ren1.AddActor(wallActor)
ren1.AddActor(finActor)
ren1.AddActor(plane1Actor)
ren1.AddActor(plane2Actor)
ren1.AddActor(plane3Actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(256,256)
cam1 = vtkCamera()
cam1.SetClippingRange(1.51176,75.5879)
cam1.SetFocalPoint(2.33749,2.96739,3.61023)
cam1.SetPosition(10.8787,5.27346,15.8687)
cam1.SetViewAngle(30)
cam1.SetViewUp(-0.0610856,0.987798,-0.143262)
ren1.SetActiveCamera(cam1)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
