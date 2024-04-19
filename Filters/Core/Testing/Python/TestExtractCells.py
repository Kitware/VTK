#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkQuadric
from vtkmodules.vtkFiltersCore import vtkExtractCells
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
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

# Test vtkExtractCells
# Control test size
#res = 200
res = 50

# Test cell extraction
#
# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtkSampleFunction()
sample.SetSampleDimensions(res,res,res)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOff()
sample.Update()

# Now extract the cells: use badly formed range
extract = vtkExtractCells()
extract.SetInputConnection(sample.GetOutputPort())
extract.AddCellRange(0,sample.GetOutput().GetNumberOfCells())

extrMapper = vtkDataSetMapper()
extrMapper.SetInputConnection(extract.GetOutputPort())
extrMapper.ScalarVisibilityOff()

extrActor = vtkActor()
extrActor.SetMapper(extrMapper)
extrActor.GetProperty().SetColor(.8,.4,.4)

# Extract interior range of cells
extract2 = vtkExtractCells()
extract2.SetInputConnection(sample.GetOutputPort())
extract2.AddCellRange(100,5000)
extract2.Update()

extr2Mapper = vtkDataSetMapper()
extr2Mapper.SetInputConnection(extract2.GetOutputPort())
extr2Mapper.ScalarVisibilityOff()

extr2Actor = vtkActor()
extr2Actor.SetMapper(extr2Mapper)
extr2Actor.GetProperty().SetColor(.8,.4,.4)

# Define graphics objects
renWin = vtkRenderWindow()
renWin.SetSize(300,150)

ren1 = vtkRenderer()
ren1.SetViewport(0,0,0.5,1)
ren1.SetBackground(1,1,1)
cam1 = ren1.GetActiveCamera()
ren2 = vtkRenderer()
ren2.SetViewport(0.5,0,1,1)
ren2.SetBackground(1,1,1)
ren2.SetActiveCamera(cam1)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(extrActor)
ren2.AddActor(extr2Actor)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
