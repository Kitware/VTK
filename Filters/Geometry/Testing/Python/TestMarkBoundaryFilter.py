#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkImageData,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import vtkThreshold
from vtkmodules.vtkFiltersExtraction import vtkExtractGeometry
from vtkmodules.vtkFiltersGeometry import vtkMarkBoundaryFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
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

# Mark the boundary and faces for different dataset types.
# Here we focus on 2D types.

# Control test resolution
res = 50

# Test with 2D data
plane = vtkPlaneSource()
plane.SetResolution(res,res)

# polydata
mark1 = vtkMarkBoundaryFilter()
mark1.SetInputConnection(plane.GetOutputPort())
mark1.Update()

thresh1 = vtkThreshold()
thresh1.SetInputConnection(mark1.GetOutputPort())
thresh1.SetThresholdFunction(vtkThreshold.THRESHOLD_UPPER)
thresh1.SetUpperThreshold(1.0)
thresh1.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_CELLS, "BoundaryCells")
thresh1.Update()

mapper1 = vtkDataSetMapper()
mapper1.SetInputConnection(thresh1.GetOutputPort())

actor1 = vtkActor()
actor1.SetMapper(mapper1)

# unstructured grid
sphere = vtkSphere()
sphere.SetCenter(0,0,0)
sphere.SetRadius(1000000)

toUG = vtkExtractGeometry()
toUG.SetInputConnection(plane.GetOutputPort())
toUG.SetImplicitFunction(sphere)

mark2 = vtkMarkBoundaryFilter()
mark2.SetInputConnection(toUG.GetOutputPort())
mark2.Update()

thresh2 = vtkThreshold()
thresh2.SetInputConnection(mark2.GetOutputPort())
thresh2.SetThresholdFunction(vtkThreshold.THRESHOLD_UPPER)
thresh2.SetUpperThreshold(1.0)
thresh2.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_CELLS, "BoundaryCells")

mapper2 = vtkDataSetMapper()
mapper2.SetInputConnection(thresh2.GetOutputPort())

actor2 = vtkActor()
actor2.SetMapper(mapper2)

# structured 2D data
image = vtkImageData()
image.SetDimensions(res,res,1)
image.SetOrigin(-0.5,-0.5,0.0)
image.SetSpacing(1.0/float(res-1),1.0/float(res-1),1.0)

mark3 = vtkMarkBoundaryFilter()
mark3.SetInputData(image)

thresh3 = vtkThreshold()
thresh3.SetInputConnection(mark3.GetOutputPort())
thresh3.SetThresholdFunction(vtkThreshold.THRESHOLD_UPPER)
thresh3.SetUpperThreshold(1.0)
thresh3.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_CELLS, "BoundaryCells")

mapper3 = vtkDataSetMapper()
mapper3.SetInputConnection(thresh3.GetOutputPort())

actor3 = vtkActor()
actor3.SetMapper(mapper3)

# Define graphics objects
ren1 = vtkRenderer()
ren1.SetViewport(0,0, 0.333,1)
ren1.SetBackground(0.5,0.5,0.5)
ren1.AddActor(actor1)

ren2 = vtkRenderer()
ren2.SetViewport(0.333,0, 0.667,1)
ren2.SetBackground(0.5,0.5,0.5)
ren2.AddActor(actor2)

ren3 = vtkRenderer()
ren3.SetViewport(0.667,0, 1,1)
ren3.SetBackground(0.5,0.5,0.5)
ren3.AddActor(actor3)

ren1.GetActiveCamera().SetPosition(0,0,1)
ren1.ResetCamera()

renWin = vtkRenderWindow()
renWin.SetSize(450,150)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
iren.Start()
# --- end of script --
