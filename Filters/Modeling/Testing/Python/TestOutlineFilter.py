#!/usr/bin/env python
import vtk

# Test vtkOutlineFilter with composite dataset input; test vtkCompositeOutlineFilter.

# Control test size
res = 50

#
# Quadric definition
quadricL = vtk.vtkQuadric()
quadricL.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sampleL = vtk.vtkSampleFunction()
sampleL.SetModelBounds(-1,0, -1,1, -1,1)
sampleL.SetSampleDimensions(int(res/2),res,res)
sampleL.SetImplicitFunction(quadricL)
sampleL.ComputeNormalsOn()
sampleL.Update()

#
# Quadric definition
quadricR = vtk.vtkQuadric()
quadricR.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sampleR = vtk.vtkSampleFunction()
sampleR.SetModelBounds(0,1, -1,1, -1,1)
sampleR.SetSampleDimensions(int(res/2),res,res)
sampleR.SetImplicitFunction(quadricR)
sampleR.ComputeNormalsOn()
sampleR.Update()

#
# Extract voxel cells
extractL = vtk.vtkExtractCells()
extractL.SetInputConnection(sampleL.GetOutputPort())
extractL.AddCellRange(0,sampleL.GetOutput().GetNumberOfCells())
extractL.Update()

#
# Extract voxel cells
extractR = vtk.vtkExtractCells()
extractR.SetInputConnection(sampleR.GetOutputPort())
extractR.AddCellRange(0,sampleR.GetOutput().GetNumberOfCells())
extractR.Update()

# Create a composite dataset. Throw in an extra polydata.
sphere = vtk.vtkSphereSource()
sphere.SetCenter(1,0,0)
sphere.Update()

composite = vtk.vtkMultiBlockDataSet()
composite.SetBlock(0,extractL.GetOutput())
composite.SetBlock(1,extractR.GetOutput())
composite.SetBlock(2,sphere.GetOutput())

# Create an outline around everything (composite dataset)
outlineF = vtk.vtkOutlineFilter()
outlineF.SetInputData(composite)
outlineF.SetCompositeStyleToRoot()
outlineF.GenerateFacesOn()

outlineM = vtk.vtkPolyDataMapper()
outlineM.SetInputConnection(outlineF.GetOutputPort())

outline = vtk.vtkActor()
outline.SetMapper(outlineM)
outline.GetProperty().SetColor(1,0,0)

# Create an outline around composite pieces
coutlineF = vtk.vtkOutlineFilter()
coutlineF.SetInputData(composite)
coutlineF.SetCompositeStyleToLeafs()

coutlineM = vtk.vtkPolyDataMapper()
coutlineM.SetInputConnection(coutlineF.GetOutputPort())

coutline = vtk.vtkActor()
coutline.SetMapper(coutlineM)
coutline.GetProperty().SetColor(0,0,0)

# Create an outline around root and leafs
outlineF2 = vtk.vtkOutlineFilter()
outlineF2.SetInputData(composite)
outlineF2.SetCompositeStyleToRootAndLeafs()

outlineM2 = vtk.vtkPolyDataMapper()
outlineM2.SetInputConnection(outlineF2.GetOutputPort())

outline2 = vtk.vtkActor()
outline2.SetMapper(outlineM2)
outline2.GetProperty().SetColor(0,0,1)

# Create an outline around root and leafs
outlineF3 = vtk.vtkOutlineFilter()
outlineF3.SetInputData(composite)
outlineF3.SetCompositeStyleToSpecifiedIndex()
outlineF3.AddIndex(3)
outlineF3.GenerateFacesOn()

outlineM3 = vtk.vtkPolyDataMapper()
outlineM3.SetInputConnection(outlineF3.GetOutputPort())

outline3 = vtk.vtkActor()
outline3.SetMapper(outlineM3)
outline3.GetProperty().SetColor(0,1,0)

# Define graphics objects
renWin = vtk.vtkRenderWindow()
renWin.SetSize(600,150)
renWin.SetMultiSamples(0)

ren1 = vtk.vtkRenderer()
ren1.SetBackground(1,1,1)
ren1.SetViewport(0,0,0.25,1)
cam1 = ren1.GetActiveCamera()
ren2 = vtk.vtkRenderer()
ren2.SetBackground(1,1,1)
ren2.SetViewport(0.25,0,0.5,1)
ren2.SetActiveCamera(cam1)
ren3 = vtk.vtkRenderer()
ren3.SetBackground(1,1,1)
ren3.SetViewport(0.5,0,0.75,1)
ren3.SetActiveCamera(cam1)
ren4 = vtk.vtkRenderer()
ren4.SetBackground(1,1,1)
ren4.SetViewport(0.75,0,1,1)
ren4.SetActiveCamera(cam1)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)

iren = vtk.vtkRenderWindowInteractor()
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
