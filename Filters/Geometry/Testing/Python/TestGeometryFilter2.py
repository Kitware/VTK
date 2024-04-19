#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkImageData,
    vtkPolyData,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import vtkSimpleElevationFilter
from vtkmodules.vtkFiltersExtraction import vtkExtractGeometry
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
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

# Test and compare vtkGeometryFilter verus
# vtkDataSetSurfaceFilter.

# Control test resolution
res = 10

# Create a synthetic source, then convert to unstructured grid
vol = vtkImageData()
vol.SetDimensions(res,res,res)

sphere = vtkSphere()
sphere.SetRadius(10000)

grid = vtkExtractGeometry()
grid.SetInputData(vol)
grid.SetImplicitFunction(sphere)
grid.Update()
print("Processing {0} hexes".format(grid.GetOutput().GetNumberOfCells()))

# Create a scalar field
ele = vtkSimpleElevationFilter()
ele.SetInputConnection(grid.GetOutputPort())

# Extract the surface with vtkGeometryFilter and time it. Indicate
# some faces to not not extract.
# the fast extraction mode.
face = [0,1,11,10]
faces = vtkCellArray()
faces.InsertNextCell(4,face)
excludedFaces = vtkPolyData()
excludedFaces.SetPolys(faces)

geomF = vtkGeometryFilter()
geomF.SetInputConnection(ele.GetOutputPort())
geomF.SetExcludedFacesData(excludedFaces)

# Show the result
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(geomF.GetOutputPort())
mapper.SetScalarRange(0,float(res-1))

actor = vtkActor()
actor.SetMapper(mapper)

# Define graphics objects
ren1 = vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.AddActor(actor)
ren1.GetActiveCamera().SetPosition(-1,-1,-1)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(2)

renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
iren.Start()
# --- end of script --
