#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkSphere
from vtkmodules.vtkCommonExecutionModel import vtkImageToStructuredPoints
from vtkmodules.vtkFiltersCore import vtkThreshold
from vtkmodules.vtkFiltersGeneral import vtkSubPixelPositionEdgels
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkImagingGeneral import vtkImageGradient
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkImagingMath import vtkImageMathematics
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

# Test sub pixel positioning (A round about way of getting an iso surface.)
# See cubed sphere for the surface before sub pixel positioning.
sphere = vtkSphere()
sphere.SetCenter(1,1,1)
sphere.SetRadius(0.9)
sample = vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(0,2,0,2,0,2)
sample.SetSampleDimensions(30,30,30)
sample.ComputeNormalsOff()
sample.Update()
threshold1 = vtkThreshold()
threshold1.SetInputConnection(sample.GetOutputPort())
threshold1.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)
threshold1.SetLowerThreshold(0.001)
geometry = vtkGeometryFilter()
geometry.SetInputConnection(threshold1.GetOutputPort())
grad = vtkImageGradient()
grad.SetDimensionality(3)
grad.SetInputConnection(sample.GetOutputPort())
grad.Update()
mult = vtkImageMathematics()
mult.SetOperationToMultiply()
mult.SetInput1Data(sample.GetOutput())
mult.SetInput2Data(sample.GetOutput())
itosp = vtkImageToStructuredPoints()
itosp.SetInputConnection(mult.GetOutputPort())
itosp.SetVectorInputData(grad.GetOutput())
itosp.Update()
sub = vtkSubPixelPositionEdgels()
sub.SetInputConnection(geometry.GetOutputPort())
sub.SetGradMapsData(itosp.GetOutput())
mapper = vtkDataSetMapper()
mapper.SetInputConnection(sub.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
# Create renderer stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(20)
ren1.GetActiveCamera().Elevation(30)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
