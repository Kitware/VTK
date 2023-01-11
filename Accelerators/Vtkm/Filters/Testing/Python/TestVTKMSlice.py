from vtkmodules.vtkAcceleratorsVTKmFilters import (
    vtkmClip,
    vtkmSlice,
)
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
import math

# Creates an unstructured grid dataset with different types of cells:
#-----------------------------------------------------------------------------
wavelet = vtkRTAnalyticSource()
wavelet.SetWholeExtent(-4, 4, -4, 4, -4, 4)

clipPlane = vtkPlane()
clipPlane.SetOrigin(0, 0, 0)
clipPlane.SetNormal(0.93, 0.363, 0.053)

clip = vtkmClip()
clip.SetInputConnection(wavelet.GetOutputPort())
clip.SetClipFunction(clipPlane)
clip.SetInsideOut(True)
#-----------------------------------------------------------------------------

center = [-1.0, 0.0, 0.0]
radius = 4
sliceSphere = vtkSphere()
sliceSphere.SetCenter(center)
sliceSphere.SetRadius(radius)

slicer = vtkmSlice()
slicer.SetInputConnection(clip.GetOutputPort())
slicer.SetCutFunction(sliceSphere)
slicer.Update()

result = slicer.GetOutput()

print("Number of Cells: ", result.GetNumberOfCells())
print("Number of Points: ", result.GetNumberOfPoints())
# distance from the center to a point should be approximately equal to the radius
for i in range(0, result.GetNumberOfPoints()):
  pt = result.GetPoint(i)
  dsqr = 0
  for c in [(x[0] - x[1])**2 for x in zip(pt, center)]:
    dsqr += c
  d = math.sqrt(dsqr)
  assert(abs(d - radius) < 0.1)

resultBounds = result.GetBounds()
print ("Bounds: ", resultBounds)
expectedBounds = [ -4, 1.273, -4, 3.957, -4, 4 ]
for x in zip(resultBounds, expectedBounds):
  assert(abs(x[0] - x[1]) < 1e-2)
