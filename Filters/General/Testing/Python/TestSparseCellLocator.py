#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    reference,
    vtkDoubleArray,
)
from vtkmodules.vtkCommonDataModel import (
    vtkGenericCell,
    vtkStaticCellLocator,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
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
from vtkmodules.test import Testing
import sys

# Test locators with data that is sparsely populated.  That is, points and
# cells may be widely separated.  The test creates three spheres: an outer
# bounding sphere containing two inner spheres. The inner spheres are not
# centered in the outer sphere - they are positioned side-by-side (in the +/-
# x direction). The points on the inner spheres are used to compute closest
# point / distance against the outer sphere. The result should be a smooth
# distance function on each of the inner spheres. Since the test creates two
# inner spheres, the coloring on each sphere should be mirror symmetric, with
# the blue shaded regions just touching each other.

# The default cell locator to use
cellLocClass = "vtkStaticCellLocator"

# Control the size of the test
testSize = "small"

# Process potential command line arguments. Turn this on if
# manually testing.
#numArgs = len(sys.argv)
#if numArgs >= 3:
#    testSize = sys.argv[2]
#    cellLocClass = sys.argv[1]

if testSize == "large":
    innerSphereRes = 60
    outerSphereRes = 50
    binRes = 250
elif testSize == "medium":
    innerSphereRes = 30
    outerSphereRes = 20
    binRes = 120
else:
    innerSphereRes = 15
    outerSphereRes = 10
    binRes = 25

print("Test size: ", testSize)
print("Locator tested: ", cellLocClass)

rightInnerSphere = vtkSphereSource()
rightInnerSphere.SetCenter(2.5,0,0)
rightInnerSphere.SetRadius(2.5)
rightInnerSphere.SetPhiResolution(innerSphereRes)
rightInnerSphere.SetThetaResolution(2*innerSphereRes)
rightInnerSphere.Update()
rightInnerPD = rightInnerSphere.GetOutput()

leftInnerSphere = vtkSphereSource()
leftInnerSphere.SetCenter(-2.5,0,0)
leftInnerSphere.SetRadius(2.5)
leftInnerSphere.SetPhiResolution(innerSphereRes)
leftInnerSphere.SetThetaResolution(2*innerSphereRes)
leftInnerSphere.Update()
leftInnerPD = leftInnerSphere.GetOutput()

outerSphere = vtkSphereSource()
outerSphere.SetCenter(0.0,0,0)
outerSphere.SetRadius(10)
outerSphere.SetPhiResolution(outerSphereRes)
outerSphere.SetThetaResolution(2*outerSphereRes)
outerSphere.Update()
outerPD = outerSphere.GetOutput()

# Now drive the locator.
instantiationString = cellLocClass + "()"
cellLoc = eval(instantiationString)
cellLoc.SetDataSet(outerPD)
cellLoc.SetNumberOfCellsPerNode(1)
cellLoc.BuildLocator()

x = [0,0,0]
genCell = vtkGenericCell()
closestPt = [0,0,0]
closestCellId = reference(-1)
subId = reference(0)
dist2 = reference(0.0)

numInnerPts = rightInnerPD.GetNumberOfPoints()
rightInnerDA = vtkDoubleArray()
rightInnerDA.SetNumberOfTuples(numInnerPts)
leftInnerDA = vtkDoubleArray()
leftInnerDA.SetNumberOfTuples(numInnerPts)

for pId in range(0,numInnerPts) :
    rightInnerPD.GetPoint(pId,x)
    cellLoc.FindClosestPoint(x, closestPt, genCell, closestCellId, subId, dist2)
    rightInnerDA.SetTuple1(pId,dist2)
    leftInnerPD.GetPoint(pId,x)
    cellLoc.FindClosestPoint(x, closestPt, genCell, closestCellId, subId, dist2)
    leftInnerDA.SetTuple1(pId,dist2)

rightInnerPD.GetPointData().SetScalars(rightInnerDA)
leftInnerPD.GetPointData().SetScalars(leftInnerDA)

# Display that sucker
rightInnerMapper = vtkPolyDataMapper()
rightInnerMapper.SetInputData(rightInnerPD)
rightInnerMapper.SetScalarRange(rightInnerDA.GetRange())

rightInnerActor = vtkActor()
rightInnerActor.SetMapper(rightInnerMapper)

leftInnerMapper = vtkPolyDataMapper()
leftInnerMapper.SetInputData(leftInnerPD)
leftInnerMapper.SetScalarRange(leftInnerDA.GetRange())

leftInnerActor = vtkActor()
leftInnerActor.SetMapper(leftInnerMapper)

# Display that sucker
outerMapper = vtkPolyDataMapper()
outerMapper.SetInputData(outerPD)
outerMapper.ScalarVisibilityOff()

outerActor = vtkActor()
outerActor.SetMapper(outerMapper)
outerActor.GetProperty().SetRepresentationToWireframe()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(rightInnerActor)
ren1.AddActor(leftInnerActor)
#ren1.AddActor(outerActor)

ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
