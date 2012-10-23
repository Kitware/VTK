#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Example demonstrates how to generate a 3D tetrahedra mesh from a
# volume. This example differs from the previous clipVolume.tcl examples
# in that it uses the slower ordered triangulator and generates clip scalars.
# Quadric definition
quadric = vtk.vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtk.vtkSampleFunction()
sample.SetSampleDimensions(20,20,20)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
sample.Update()
# Program a bandpass filter to clip a range of data. What we do is transform the
# scalars so that values laying betweeen (minRange,maxRange) are >= 0.0; all
# others are < 0.0,
dataset = vtk.vtkImplicitDataSet()
dataset.SetDataSet(sample.GetOutput())
window = vtk.vtkImplicitWindowFunction()
window.SetImplicitFunction(dataset)
window.SetWindowRange(0.5,1.0)
# Generate tetrahedral mesh
clip = vtk.vtkClipVolume()
clip.SetInputConnection(sample.GetOutputPort())
clip.SetClipFunction(window)
clip.SetValue(0.0)
clip.GenerateClippedOutputOff()
clip.Mixed3DCellGenerationOff()
gf = vtk.vtkGeometryFilter()
#  gf SetInput [clip GetClippedOutput]
gf.SetInputConnection(clip.GetOutputPort())
clipMapper = vtk.vtkPolyDataMapper()
clipMapper.SetInputConnection(gf.GetOutputPort())
clipMapper.ScalarVisibilityOn()
clipMapper.SetScalarRange(0,2)
clipActor = vtk.vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(.8,.4,.4)
# Create outline
outline = vtk.vtkOutlineFilter()
#  outline SetInputData [clip GetInput]
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Define graphics objects
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(1,1,1)
ren1.AddActor(clipActor)
ren1.AddActor(outlineActor)
iren.Initialize()
# --- end of script --
