#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
    vtkSphere,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractGeometry
from vtkmodules.vtkFiltersGeometry import vtkAttributeSmoothingFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkImagingSources import vtkImageMandelbrotSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the vtkAttributeSmoothingFilter. This test
# focused on testing general datasets.

# Control test size
res = 25

# Generate synthetic attributes for 2D unstructured grid
#
ps = vtkPlaneSource()
ps.SetXResolution(res-1)
ps.SetYResolution(res-1)
ps.Update()

pd = vtkPolyData()
pts = vtkPoints()
polys = vtkCellArray()
pd.SetPoints(ps.GetOutput().GetPoints())
pd.SetPolys(ps.GetOutput().GetPolys())

# Create some synthetic attribute data
s = vtkFloatArray()
s.SetNumberOfTuples(res*res)
s.Fill(4) # Interior all the same value
pd.GetPointData().SetScalars(s)

def SetRow(row,value):
    for x in range(0,res):
        idx = x + row*res
        s.SetTuple1(idx,value)

def SetColumn(col,value):
    for y in range(0,res):
        idx = col + y*res
        s.SetTuple1(idx,value)

def SetPoint(col,row,value):
    idx = col + row*res
    s.SetTuple1(idx,value)

# Set values adjacent to boundary
SetRow(1,1)
SetRow(res-2,1)
SetColumn(1,1)
SetColumn(res-2,1)

# Set boundary values
SetRow(0,0)
SetRow(res-1,0)
SetColumn(0,0)
SetColumn(res-1,0)

# Set center point
SetPoint(int(res/2),int(res/2),0)

# Convert this vtkPolyData to an unstructured grid
sphere = vtkSphere()
sphere.SetCenter(0,0,0)
sphere.SetRadius(1000000)

extract = vtkExtractGeometry()
extract.SetInputData(pd)
extract.SetImplicitFunction(sphere)
extract.Update()

# Display what we've got
pdm0 = vtkDataSetMapper()
pdm0.SetInputConnection(extract.GetOutputPort())
pdm0.SetScalarRange(0,4)

a0 = vtkActor()
a0.SetMapper(pdm0)

# Smooth the 2D attribute data
att1 = vtkAttributeSmoothingFilter()
att1.SetInputConnection(extract.GetOutputPort())
att1.SetSmoothingStrategyToAllButBoundary()
att1.SetNumberOfIterations(20)
att1.SetRelaxationFactor(0.1)
att1.Update()

pdm1 = vtkDataSetMapper()
pdm1.SetInputConnection(att1.GetOutputPort())
pdm1.SetScalarRange(0,4)

a1 = vtkActor()
a1.SetMapper(pdm1)

# Now smooth 3D attributes
mandel = vtkImageMandelbrotSource()
mandel.SetWholeExtent(-res,res,-res,res,-res,res)
mandel.Update()

pdm2 = vtkDataSetMapper()
pdm2.SetInputConnection(mandel.GetOutputPort())
pdm2.SetScalarRange(mandel.GetOutput().GetScalarRange())
pdm2.SetScalarRange(1,8)

a2 = vtkActor()
a2.SetMapper(pdm2)

# Now smooth 3D attributes
att3 = vtkAttributeSmoothingFilter()
att3.SetInputConnection(mandel.GetOutputPort())
att3.SetSmoothingStrategyToAllPoints()
att3.SetNumberOfIterations(50)
att3.SetRelaxationFactor(0.1)
att3.Update()

pdm3 = vtkDataSetMapper()
pdm3.SetInputConnection(att3.GetOutputPort())
pdm3.SetScalarRange(mandel.GetOutput().GetScalarRange())
pdm3.SetScalarRange(1,8)

a3 = vtkActor()
a3.SetMapper(pdm3)


# Create the RenderWindow, Renderer and interactive renderer
#
renWin = vtkRenderWindow()
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,0.5)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1,0.5)
ren2 = vtkRenderer()
ren2.SetViewport(0,0.5,0.5,1)
ren3 = vtkRenderer()
ren3.SetViewport(0.5,0.5,1,1)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)

# make sure to have the same regression image on all platforms.
renWin.SetMultiSamples(0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren0.AddActor(a0)
ren0.SetBackground(0, 0, 0)
ren1.AddActor(a1)
ren1.SetBackground(0, 0, 0)
ren2.AddActor(a2)
ren2.SetBackground(0, 0, 0)
ren2.GetActiveCamera().SetPosition(-1,1,1)
ren2.ResetCamera()
ren3.AddActor(a3)
ren3.SetBackground(0, 0, 0)
ren3.SetActiveCamera(ren2.GetActiveCamera())

renWin.SetSize(400, 400)

renWin.Render()
iren.Start()
