#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkImplicitDataSet,
    vtkImplicitWindowFunction,
    vtkQuadric,
)
from vtkmodules.vtkFiltersGeneral import vtkClipVolume
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Example demonstrates how to generate a 3D tetrahedra mesh from a
# volume. This example differs from the previous clipVolume.tcl examples
# in that it uses the slower ordered triangulator and generates clip scalars.
# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtkSampleFunction()
sample.SetSampleDimensions(20,20,20)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
sample.Update()
# Program a bandpass filter to clip a range of data. What we do is transform the
# scalars so that values laying between (minRange,maxRange) are >= 0.0; all
# others are < 0.0,
dataset = vtkImplicitDataSet()
dataset.SetDataSet(sample.GetOutput())
window = vtkImplicitWindowFunction()
window.SetImplicitFunction(dataset)
window.SetWindowRange(0.5,1.0)
# Generate tetrahedral mesh
clip = vtkClipVolume()
clip.SetInputConnection(sample.GetOutputPort())
clip.SetClipFunction(window)
clip.SetValue(0.0)
clip.GenerateClippedOutputOff()
clip.Mixed3DCellGenerationOff()
gf = vtkGeometryFilter()
#  gf SetInput [clip GetClippedOutput]
gf.SetInputConnection(clip.GetOutputPort())
clipMapper = vtkPolyDataMapper()
clipMapper.SetInputConnection(gf.GetOutputPort())
clipMapper.ScalarVisibilityOn()
clipMapper.SetScalarRange(0,2)
clipActor = vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(.8,.4,.4)
# Create outline
outline = vtkOutlineFilter()
#  outline SetInputData [clip GetInput]
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Define graphics objects
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(1,1,1)
ren1.AddActor(clipActor)
ren1.AddActor(outlineActor)
iren.Initialize()
# --- end of script --
