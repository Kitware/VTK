#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

math = vtk.vtkMath()

# Generate some random colors
def MakeColors (lut, n):
    lut.SetNumberOfColors(n)
    lut.SetTableRange(0, n - 1)
    lut.SetScaleToLinear()
    lut.Build()
    lut.SetTableValue(0, 0, 0, 0, 1)
    math.RandomSeed(5071)
    i = 1
    while i < n:
        lut.SetTableValue(i, math.Random(.2, 1),
          math.Random(.2, 1), math.Random(.2, 1), 1)
        i += 1

lut = vtk.vtkLookupTable()
MakeColors(lut, 256)
n = 20
radius = 10

# This has been moved outside the loop so that the code can be correctly
# translated to python
blobImage = vtk.vtkImageData()

i = 0
while i < n:
    sphere = vtk.vtkSphere()
    sphere.SetRadius(radius)
    max = 50 - radius
    sphere.SetCenter(int(math.Random(-max, max)),
      int(math.Random(-max, max)), int(math.Random(-max, max)))

    sampler = vtk.vtkSampleFunction()
    sampler.SetImplicitFunction(sphere)
    sampler.SetOutputScalarTypeToFloat()
    sampler.SetSampleDimensions(51, 51, 51)
    sampler.SetModelBounds(-50, 50, -50, 50, -50, 50)

    thres = vtk.vtkImageThreshold()
    thres.SetInputConnection(sampler.GetOutputPort())
    thres.ThresholdByLower(radius * radius)
    thres.ReplaceInOn()
    thres.ReplaceOutOn()
    thres.SetInValue(i + 1)
    thres.SetOutValue(0)
    thres.Update()
    if (i == 0):
        blobImage.DeepCopy(thres.GetOutput())

    maxValue = vtk.vtkImageMathematics()
    maxValue.SetInputData(0, blobImage)
    maxValue.SetInputData(1, thres.GetOutput())
    maxValue.SetOperationToMax()
    maxValue.Modified()
    maxValue.Update()

    blobImage.DeepCopy(maxValue.GetOutput())

    i += 1

discrete = vtk.vtkDiscreteMarchingCubes()
discrete.SetInputData(blobImage)
discrete.GenerateValues(n, 1, n)
discrete.ComputeAdjacentScalarsOn() # creates PointScalars

thr = vtk.vtkThreshold()
thr.SetInputConnection(discrete.GetOutputPort())
thr.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, vtk.vtkDataSetAttributes.SCALARS) # act on PointScalars created by ComputeAdjacentScalarsOn
thr.AllScalarsOn() # default, changes better visible
thr.ThresholdBetween(0, 0) # remove cells between labels, i.e. keep cells neighbouring background (label 0)

vtu2vtp = vtk.vtkGeometryFilter()
vtu2vtp.SetInputConnection(thr.GetOutputPort())

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(vtu2vtp.GetOutputPort())
mapper.SetLookupTable(lut)
mapper.SetScalarModeToUseCellData() # default is to use PointScalars, which get created with ComputeAdjacentScalarsOn
mapper.SetScalarRange(0, lut.GetNumberOfColors())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

ren1.AddActor(actor)

renWin.Render()

#iren.Start()
