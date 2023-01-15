#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkRectilinearSynchronizedTemplates
from vtkmodules.vtkIOLegacy import vtkRectilinearGridReader
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

# create pipeline - rectilinear grid
#
rgridReader = vtkRectilinearGridReader()
rgridReader.SetFileName(VTK_DATA_ROOT + "/Data/RectGrid2.vtk")
rgridReader.Update()
contour = vtkRectilinearSynchronizedTemplates()
contour.SetInputConnection(rgridReader.GetOutputPort())
contour.SetValue(0,1)
contour.ComputeScalarsOff()
contour.ComputeNormalsOn()
contour.ComputeGradientsOn()
cMapper = vtkPolyDataMapper()
cMapper.SetInputConnection(contour.GetOutputPort())
cActor = vtkActor()
cActor.SetMapper(cMapper)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(200,200)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(cActor)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
