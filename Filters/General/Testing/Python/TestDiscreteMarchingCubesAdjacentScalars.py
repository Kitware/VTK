#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkLookupTable,
    vtkMath,
)
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkDataSetAttributes,
    vtkImageData,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import vtkThreshold
from vtkmodules.vtkFiltersGeneral import vtkDiscreteMarchingCubes
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkImagingCore import vtkImageThreshold
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkImagingMath import vtkImageMathematics
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

math = vtkMath()

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

lut = vtkLookupTable()
MakeColors(lut, 256)
n = 20
radius = 10

# This has been moved outside the loop so that the code can be correctly
# translated to python
blobImage = vtkImageData()

i = 0
while i < n:
    sphere = vtkSphere()
    sphere.SetRadius(radius)
    max = 50 - radius
    sphere.SetCenter(int(math.Random(-max, max)),
      int(math.Random(-max, max)), int(math.Random(-max, max)))

    sampler = vtkSampleFunction()
    sampler.SetImplicitFunction(sphere)
    sampler.SetOutputScalarTypeToFloat()
    sampler.SetSampleDimensions(51, 51, 51)
    sampler.SetModelBounds(-50, 50, -50, 50, -50, 50)

    thres = vtkImageThreshold()
    thres.SetInputConnection(sampler.GetOutputPort())
    thres.ThresholdByLower(radius * radius)
    thres.ReplaceInOn()
    thres.ReplaceOutOn()
    thres.SetInValue(i + 1)
    thres.SetOutValue(0)
    thres.Update()
    if (i == 0):
        blobImage.DeepCopy(thres.GetOutput())

    maxValue = vtkImageMathematics()
    maxValue.SetInputData(0, blobImage)
    maxValue.SetInputData(1, thres.GetOutput())
    maxValue.SetOperationToMax()
    maxValue.Modified()
    maxValue.Update()

    blobImage.DeepCopy(maxValue.GetOutput())

    i += 1

discrete = vtkDiscreteMarchingCubes()
discrete.SetInputData(blobImage)
discrete.GenerateValues(n, 1, n)
discrete.ComputeAdjacentScalarsOn() # creates PointScalars

thr = vtkThreshold()
thr.SetInputConnection(discrete.GetOutputPort())
thr.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes.SCALARS) # act on PointScalars created by ComputeAdjacentScalarsOn
thr.AllScalarsOn() # default, changes better visible

# remove cells between labels, i.e. keep cells neighbouring background (label 0)
thr.SetThresholdFunction(vtkThreshold.THRESHOLD_BETWEEN)
thr.SetLowerThreshold(0.0)
thr.SetUpperThreshold(0.0)

vtu2vtp = vtkGeometryFilter()
vtu2vtp.SetInputConnection(thr.GetOutputPort())

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(vtu2vtp.GetOutputPort())
mapper.SetLookupTable(lut)
mapper.SetScalarModeToUseCellData() # default is to use PointScalars, which get created with ComputeAdjacentScalarsOn
mapper.SetScalarRange(0, lut.GetNumberOfColors())

actor = vtkActor()
actor.SetMapper(mapper)

ren1.AddActor(actor)

renWin.Render()

#iren.Start()
