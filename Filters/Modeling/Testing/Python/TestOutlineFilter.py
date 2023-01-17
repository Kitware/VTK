#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkMultiBlockDataSet,
    vtkQuadric,
)
from vtkmodules.vtkFiltersCore import vtkExtractCells
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
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

# Test vtkOutlineFilter with composite dataset input; test vtkCompositeOutlineFilter.

# Control test size
res = 50

#
# Quadric definition
quadricL = vtkQuadric()
quadricL.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sampleL = vtkSampleFunction()
sampleL.SetModelBounds(-1,0, -1,1, -1,1)
sampleL.SetSampleDimensions(int(res/2),res,res)
sampleL.SetImplicitFunction(quadricL)
sampleL.ComputeNormalsOn()
sampleL.Update()

#
# Quadric definition
quadricR = vtkQuadric()
quadricR.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sampleR = vtkSampleFunction()
sampleR.SetModelBounds(0,1, -1,1, -1,1)
sampleR.SetSampleDimensions(int(res/2),res,res)
sampleR.SetImplicitFunction(quadricR)
sampleR.ComputeNormalsOn()
sampleR.Update()

#
# Extract voxel cells
extractL = vtkExtractCells()
extractL.SetInputConnection(sampleL.GetOutputPort())
extractL.AddCellRange(0,sampleL.GetOutput().GetNumberOfCells())
extractL.Update()

#
# Extract voxel cells
extractR = vtkExtractCells()
extractR.SetInputConnection(sampleR.GetOutputPort())
extractR.AddCellRange(0,sampleR.GetOutput().GetNumberOfCells())
extractR.Update()

# Create a composite dataset. Throw in an extra polydata.
sphere = vtkSphereSource()
sphere.SetCenter(1,0,0)
sphere.Update()

composite = vtkMultiBlockDataSet()
composite.SetBlock(0,extractL.GetOutput())
composite.SetBlock(1,extractR.GetOutput())
composite.SetBlock(2,sphere.GetOutput())

# Create an outline around everything (composite dataset)
outlineF = vtkOutlineFilter()
outlineF.SetInputData(composite)
outlineF.SetCompositeStyleToRoot()
outlineF.GenerateFacesOn()

outlineM = vtkPolyDataMapper()
outlineM.SetInputConnection(outlineF.GetOutputPort())

outline = vtkActor()
outline.SetMapper(outlineM)
outline.GetProperty().SetColor(1,0,0)

# Create an outline around composite pieces
coutlineF = vtkOutlineFilter()
coutlineF.SetInputData(composite)
coutlineF.SetCompositeStyleToLeafs()

coutlineM = vtkPolyDataMapper()
coutlineM.SetInputConnection(coutlineF.GetOutputPort())

coutline = vtkActor()
coutline.SetMapper(coutlineM)
coutline.GetProperty().SetColor(0,0,0)

# Create an outline around root and leafs
outlineF2 = vtkOutlineFilter()
outlineF2.SetInputData(composite)
outlineF2.SetCompositeStyleToRootAndLeafs()

outlineM2 = vtkPolyDataMapper()
outlineM2.SetInputConnection(outlineF2.GetOutputPort())

outline2 = vtkActor()
outline2.SetMapper(outlineM2)
outline2.GetProperty().SetColor(0,0,1)

# Create an outline around root and leafs
outlineF3 = vtkOutlineFilter()
outlineF3.SetInputData(composite)
outlineF3.SetCompositeStyleToSpecifiedIndex()
outlineF3.AddIndex(3)
outlineF3.GenerateFacesOn()

outlineM3 = vtkPolyDataMapper()
outlineM3.SetInputConnection(outlineF3.GetOutputPort())

outline3 = vtkActor()
outline3.SetMapper(outlineM3)
outline3.GetProperty().SetColor(0,1,0)

# Define graphics objects
renWin = vtkRenderWindow()
renWin.SetSize(600,150)
renWin.SetMultiSamples(0)

ren1 = vtkRenderer()
ren1.SetBackground(1,1,1)
ren1.SetViewport(0,0,0.25,1)
cam1 = ren1.GetActiveCamera()
ren2 = vtkRenderer()
ren2.SetBackground(1,1,1)
ren2.SetViewport(0.25,0,0.5,1)
ren2.SetActiveCamera(cam1)
ren3 = vtkRenderer()
ren3.SetBackground(1,1,1)
ren3.SetViewport(0.5,0,0.75,1)
ren3.SetActiveCamera(cam1)
ren4 = vtkRenderer()
ren4.SetBackground(1,1,1)
ren4.SetViewport(0.75,0,1,1)
ren4.SetActiveCamera(cam1)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(outline)
ren2.AddActor(coutline)
ren3.AddActor(outline2)
ren4.AddActor(outline3)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
