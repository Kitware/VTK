#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Set up source for linear cells
cellSource = vtk.vtkCellTypeSource()
cellSource.SetCellType(12)
cellSource.SetBlocksDimensions(10, 10, 10)
cellSource.Update()

# Add several contour values
contour = vtk.vtkContour3DLinearGrid()
contour.SetInputConnection(cellSource.GetOutputPort())
contour.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "DistanceToCenter")
contour.SetValue(0, 4)
contour.SetValue(1, 5)
contour.SetValue(2, 6)
contour.SetMergePoints(1)
contour.SetSequentialProcessing(0)
contour.SetInterpolateAttributes(1);
contour.SetComputeNormals(1);
contour.Update()

# Ensure that the number of tuples in the output arrays matches the number of points
output = contour.GetOutput()
ptData = output.GetPointData()
for i in range(ptData.GetNumberOfArrays()):
    numTuples = ptData.GetArray(i).GetNumberOfTuples()
    numPts = output.GetNumberOfPoints()
    print("numTuples: %d numPts: %d" % (numTuples, numPts))
    assert(numTuples == numPts)
