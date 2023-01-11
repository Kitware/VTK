#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkCommonDataModel import vtkFieldData
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersGeneral import vtkTransformFilter
from vtkmodules.vtkFiltersHybrid import vtkWeightedTransformFilter
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# use a sphere as a basis of the shape
sphere = vtkSphereSource()
sphere.SetPhiResolution(40)
sphere.SetThetaResolution(40)
sphere.Update()
sphereData = sphere.GetOutput()

# create a data array to hold the weighting coefficients
tfarray = vtkFloatArray()
npoints = sphereData.GetNumberOfPoints()
tfarray.SetNumberOfComponents(2)
tfarray.SetNumberOfTuples(npoints)

# parameterize the sphere along the z axis, and fill the weights
# with (1.0-a, a) to linearly interpolate across the shape
i = 0
while i < npoints:
    pt = sphereData.GetPoint(i)
    x = pt[0]
    y = pt[1]
    z = pt[2]
    zn = z + 0.5
    zn1 = 1.0 - zn
    if (zn > 1.0):
        zn = 1.0
    if (zn1 < 0.0):
        zn1 = 0.0
    tfarray.SetComponent(i, 0, zn1)
    tfarray.SetComponent(i, 1, zn)
    i += 1

# create field data to hold the array, and bind it to the sphere
fd = vtkFieldData()
tfarray.SetName("weights")
sphereData.GetPointData().AddArray(tfarray)

# use an ordinary transform to stretch the shape
stretch = vtkTransform()
stretch.Scale(1, 1, 3.2)

stretchFilter = vtkTransformFilter()
stretchFilter.SetInputData(sphereData)
stretchFilter.SetTransform(stretch)

# now, for the weighted transform stuff
weightedTrans = vtkWeightedTransformFilter()

# create two transforms to interpolate between
identity = vtkTransform()
identity.Identity()

rotated = vtkTransform()
rotatedAngle = 45
rotated.RotateX(rotatedAngle)

weightedTrans.SetNumberOfTransforms(2)
weightedTrans.SetTransform(identity, 0)
weightedTrans.SetTransform(rotated, 1)

# which data array should the filter use ?
weightedTrans.SetWeightArray("weights")

weightedTrans.SetInputConnection(stretchFilter.GetOutputPort())

weightedTransMapper = vtkPolyDataMapper()
weightedTransMapper.SetInputConnection(weightedTrans.GetOutputPort())

weightedTransActor = vtkActor()
weightedTransActor.SetMapper(weightedTransMapper)
weightedTransActor.GetProperty().SetDiffuseColor(0.8, 0.8, 0.1)
weightedTransActor.GetProperty().SetRepresentationToSurface()

# create simple poly data so we can apply glyph
#
# Create the rendering stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(weightedTransActor)
ren1.SetBackground(0.1, 0.2, 0.5)

renWin.SetSize(300, 300)

ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(90)
ren1.GetActiveCamera().Dolly(1)

# Get handles to some useful objects
#
renWin.Render()

def cmd (s):
    rotated.Identity()
    rotated.RotateX(s)
    renWin.Render()

cmd(rotatedAngle)

iren.Initialize()
#iren.Start()
