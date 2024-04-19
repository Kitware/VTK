#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkImageData,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkSurfaceNets2D,
    vtkThreshold,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
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

# Manually create a label map. Place a bunch of
# overlapping circles and then extract the
# resulting regions.
VTK_SHORT = 4
res = 500
image = vtkImageData()
image.SetDimensions(res,res,1)
image.AllocateScalars(VTK_SHORT,1)

imMapper = vtkDataSetMapper()
imMapper.SetInputData(image)
imMapper.ScalarVisibilityOn()
imMapper.SetScalarRange(0,5)

imActor = vtkActor()
imActor.SetMapper(imMapper)

# Fill the scalars with 0 and then set particular values.
# Here we'll create several regions / labels.
def GenIndex(i,j):
    return i + j*res

scalars = image.GetPointData().GetScalars()
scalars.Fill(0)

# Generate a circle of labels
def GenCircle(res,center,radius,label):
    radius2 = radius * radius
    for y in range(0,res):
        for x in range(0,res):
            r2 = (x-center[0])*(x-center[0]) + (y-center[1])*(y-center[1])
            if r2 <= radius2:
                scalars.SetTuple1(GenIndex(x,y),label)

# Place circles
center = [ res/2.5, res/1.75 ]
radius = res/9
GenCircle(res,center,radius,5)

center = [ res/2, res/3 ]
radius = res/4
GenCircle(res,center,radius,1)

center = [ res/2, res/2 ]
radius = res/6
GenCircle(res,center,radius,2)

center = [ res/2, res/1.5 ]
radius = res/9
GenCircle(res,center,radius,3)

center = [ res/2, res/1.25 ]
radius = res/12
GenCircle(res,center,radius,4)

center = [ res/1.5, res/1.75 ]
radius = res/9
GenCircle(res,center,radius,5)


# Extract the boundaries of labels 1-5 with SurfaceNets
snets = vtkSurfaceNets2D()
snets.SetInputData(image)
snets.SetValue(0,1)
snets.SetValue(1,2)
snets.SetValue(2,3)
snets.SetValue(3,4)
snets.SetValue(4,5)
snets.SmoothingOn()
snets.GetSmoother().SetNumberOfIterations(100)
snets.GetSmoother().SetRelaxationFactor(0.2)
snets.GetSmoother().SetConstraintDistance(0.75)
snets.ComputeScalarsOn()
snets.SetBackgroundLabel(-1)

timer = vtkTimerLog()
timer.StartTimer()
snets.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate Surface Net: {0}".format(time))
print("Scalars: ", snets.GetOutput().GetCellData().GetArray("BoundaryLabels").GetTuple2(1000))

# Now test the smoothing caching
if snets.GetSmoothing() > 0:
    snets.GetSmoother().Modified()
    timer.StartTimer()
    snets.Update()
    timer.StopTimer()
    time = timer.GetElapsedTime()
    print("Time to smooth Surface Net: {0}".format(time))

# Clipped polygons are generated
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(snets.GetOutputPort())
mapper.ScalarVisibilityOff()

actor = vtkActor()
actor.SetMapper(mapper)

# Threshold boundary edges (i.e., edges that border background)
threshL = vtkThreshold()
threshL.SetInputConnection(snets.GetOutputPort())
threshL.SetLowerThreshold(0)
threshL.SetUpperThreshold(5)
threshL.SetInputArrayToProcess(0,0,0, vtkDataObject.FIELD_ASSOCIATION_CELLS, "BoundaryLabels")
threshL.SetComponentModeToUseAny()
threshL.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)

threshLMapper = vtkDataSetMapper()
threshLMapper.SetInputConnection(threshL.GetOutputPort())
threshLMapper.ScalarVisibilityOff()

threshLActor = vtkActor()
threshLActor.SetMapper(threshLMapper)

# Threshold boundary edges (i.e., edges that lie between two segmented
# objects)
threshU = vtkThreshold()
threshU.SetInputConnection(snets.GetOutputPort())
threshU.SetLowerThreshold(0)
threshU.SetInputArrayToProcess(0,0,0, vtkDataObject.FIELD_ASSOCIATION_CELLS, "BoundaryLabels")
threshU.SetComponentModeToUseSelected()
threshU.SetSelectedComponent(1)
threshU.InvertOn()
threshU.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)

threshUMapper = vtkDataSetMapper()
threshUMapper.SetInputConnection(threshU.GetOutputPort())
threshUMapper.ScalarVisibilityOff()

threshUActor = vtkActor()
threshUActor.SetMapper(threshUMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.SetViewport(0,0,0.333,1)
ren2 = vtkRenderer()
ren2.SetBackground(0,0,0)
ren2.SetViewport(0.333,0,0.667,1)
ren3 = vtkRenderer()
ren3.SetBackground(0,0,0)
ren3.SetViewport(.667,0,1,1)

renWin = vtkRenderWindow()
renWin.SetSize(600,200)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)
ren2.AddActor(threshLActor)
ren3.AddActor(threshUActor)
#ren1.AddActor(imActor) #uncomments to see image

ren1.ResetCamera()
ren2.SetActiveCamera(ren1.GetActiveCamera())
ren3.SetActiveCamera(ren1.GetActiveCamera())

renWin.Render()
iren.Start()
