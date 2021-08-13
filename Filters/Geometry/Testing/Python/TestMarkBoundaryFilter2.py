#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Mark the boundary and faces for different dataset types.
# Here we focus on 3D types.

# Control test resolution
res = 50

# Create a 3D volume
image = vtk.vtkImageData()
image.SetDimensions(res,res,res)
image.SetOrigin(-0.5,-0.5,-0.5)
image.SetSpacing(1.0/float(res-1),1.0/float(res-1),1.0/float(res-1))

mark1 = vtk.vtkMarkBoundaryFilter()
mark1.SetInputData(image)
mark1.GenerateBoundaryFacesOn()

thresh1 = vtk.vtkThreshold()
thresh1.SetInputConnection(mark1.GetOutputPort())
thresh1.SetThresholdFunction(vtk.vtkThreshold.THRESHOLD_UPPER)
thresh1.SetUpperThreshold(1.0)
thresh1.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_CELLS, "BoundaryCells")

mapper1 = vtk.vtkDataSetMapper()
mapper1.SetInputConnection(thresh1.GetOutputPort())
mapper1.ScalarVisibilityOff()

actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)

# unstructured grid
sphere = vtk.vtkSphere()
sphere.SetCenter(0,0,0)
sphere.SetRadius(1000000)

toUG = vtk.vtkExtractGeometry()
toUG.SetInputData(image)
toUG.SetImplicitFunction(sphere)

mark2 = vtk.vtkMarkBoundaryFilter()
mark2.SetInputConnection(toUG.GetOutputPort())
mark2.GenerateBoundaryFacesOn()
mark2.Update()

thresh2 = vtk.vtkThreshold()
thresh2.SetInputConnection(mark2.GetOutputPort())
thresh2.SetThresholdFunction(vtk.vtkThreshold.THRESHOLD_UPPER)
thresh2.SetUpperThreshold(1.0)
thresh2.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_CELLS, "BoundaryCells")

mapper2 = vtk.vtkDataSetMapper()
mapper2.SetInputConnection(thresh2.GetOutputPort())
mapper2.ScalarVisibilityOff()

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)

# Define graphics objects
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0,0, 0.5, 1)
ren1.SetBackground(0,0,0)
ren1.AddActor(actor1)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetPosition(0.25,0.5,1)
ren1.ResetCamera()

ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5,0, 1,1)
ren2.SetBackground(0,0,0)
ren2.AddActor(actor2)
ren2.SetActiveCamera(ren1.GetActiveCamera())

renWin = vtk.vtkRenderWindow()
renWin.SetSize(300,150)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
iren.Start()
# --- end of script --
