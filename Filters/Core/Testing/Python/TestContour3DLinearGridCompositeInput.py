#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkMultiBlockDataSet,
    vtkQuadric,
)
from vtkmodules.vtkCommonExecutionModel import vtkSpanSpace
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkContour3DLinearGrid,
    vtkExtractCells,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

# Test vtkContour3DLinearGrid with vtkScalarTree and an input
# vtkCompositeDataSet

# Control test size
res = 50
#res = 250
serialProcessing = 0
mergePoints = 1
interpolateAttr = 0
computeNormals = 0
computeScalarRange = 0

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

# Create a composite dataset. Throw in an extra polydata which should be skipped.
sphere = vtkSphereSource()
sphere.SetCenter(1,0,0)
sphere.Update()

composite = vtkMultiBlockDataSet()
composite.SetBlock(0,extractL.GetOutput())
composite.SetBlock(1,extractR.GetOutput())
composite.SetBlock(2,sphere.GetOutput())

# Scalar tree is used to clone for each of the composite pieces
stree = vtkSpanSpace()
stree.SetDataSet(extractL.GetOutput())
stree.ComputeResolutionOn()
stree.SetScalarRange(0.25,0.75)
stree.SetComputeScalarRange(computeScalarRange)

# Since there is a vtkPolyData added to the composite input, this dataset
# cannot be fully processed by vtkContour3DLinearGrid
assert(not vtkContour3DLinearGrid.CanFullyProcessDataObject(composite, "scalars"))

# Now contour the cells, using scalar tree
contour = vtkContour3DLinearGrid()
contour.SetInputData(composite)
contour.SetValue(0, 0.5)
contour.SetMergePoints(mergePoints)
contour.SetSequentialProcessing(serialProcessing)
contour.SetInterpolateAttributes(interpolateAttr);
contour.SetComputeNormals(computeNormals);
contour.UseScalarTreeOn()
contour.SetScalarTree(stree)

contMapper = vtkCompositePolyDataMapper()
contMapper.SetInputConnection(contour.GetOutputPort())
contMapper.ScalarVisibilityOff()

contActor = vtkActor()
contActor.SetMapper(contMapper)
contActor.GetProperty().SetColor(.8,.4,.4)

# Now contour the cells, using scalar tree
outlineF = vtkOutlineFilter()
outlineF.SetInputData(composite)

outlineM = vtkPolyDataMapper()
outlineM.SetInputConnection(outlineF.GetOutputPort())

outline = vtkActor()
outline.SetMapper(outlineM)
outline.GetProperty().SetColor(0,0,0)

# Timing comparisons
timer = vtkTimerLog()

timer.StartTimer()
contour.Update()
timer.StopTimer()
streetime = timer.GetElapsedTime()

timer.StartTimer()
contour.Modified()
contour.Update()
timer.StopTimer()
time = timer.GetElapsedTime()

print("Contouring comparison")
print("\tScalar Range: {}".format(stree.GetScalarRange()))
print("\tNumber of threads used: {0}".format(contour.GetNumberOfThreadsUsed()))
print("\tMerge points: {0}".format(mergePoints))
print("\tInterpolate attributes: {0}".format(interpolateAttr))
print("\tCompute Normals: {0}".format(computeNormals))

print("\tLinear 3D contouring (Build scalar tree): {0}".format(streetime))
print("\t\tNumber of points: {0}".format(contour.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour.GetOutput().GetNumberOfCells()))

print("\tLinear 3D contouring (With built scalar tree): {0}".format(time))
print("\t\tNumber of points: {0}".format(contour.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour.GetOutput().GetNumberOfCells()))

# Define graphics objects
renWin = vtkRenderWindow()
renWin.SetSize(200,200)
renWin.SetMultiSamples(0)

ren1 = vtkRenderer()
ren1.SetBackground(1,1,1)
cam1 = ren1.GetActiveCamera()

renWin.AddRenderer(ren1)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(contActor)
ren1.AddActor(outline)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
