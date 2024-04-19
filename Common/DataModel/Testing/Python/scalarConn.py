#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkQuadric
from vtkmodules.vtkFiltersCore import (
    vtkConnectivityFilter,
    vtkContourFilter,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
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

# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtkSampleFunction()
sample.SetSampleDimensions(30,30,30)
sample.SetImplicitFunction(quadric)
sample.Update()
#sample Print
sample.ComputeNormalsOff()
# Extract cells that contains isosurface of interest
conn = vtkConnectivityFilter()
conn.SetInputConnection(sample.GetOutputPort())
conn.ScalarConnectivityOn()
conn.SetScalarRange(0.6,0.6)
conn.SetExtractionModeToCellSeededRegions()
conn.AddSeed(105)
# Create a surface
contours = vtkContourFilter()
contours.SetInputConnection(conn.GetOutputPort())
#  contours SetInputConnection [sample GetOutputPort]
contours.GenerateValues(5,0.0,1.2)
contMapper = vtkDataSetMapper()
#  contMapper SetInputConnection [contours GetOutputPort]
contMapper.SetInputConnection(conn.GetOutputPort())
contMapper.SetScalarRange(0.0,1.2)
contActor = vtkActor()
contActor.SetMapper(contMapper)
# Create outline
outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Graphics
# create a window to render into
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
# create a renderer
# interactiver renderer catches mouse events (optional)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(1,1,1)
ren1.AddActor(contActor)
ren1.AddActor(outlineActor)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.4)
iren.Initialize()
# --- end of script --
