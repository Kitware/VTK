#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkTIFFReader
from vtkmodules.vtkRenderingCore import (
    vtkImageActor,
    vtkImageSliceMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkCommonTransforms import vtkTransform
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

pnmReader = vtkTIFFReader()
pnmReader.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")
pnmReader.SetOrientationType(4)
pnmReader.Update()
inputImage = pnmReader.GetOutput()

# Modify the image origin with very large values to test precision
origin = [999999999.12, 999999999.12, 999999999.12]
inputImage.SetOrigin(origin)

# Apply a rotation to the image direction matrix, to expose precision issues
transform = vtkTransform()
transform.RotateX(15.0)
transform.RotateZ(16.0)
m = transform.GetMatrix()
inputImage.SetDirectionMatrix(m.GetElement(0, 0), m.GetElement(0, 1), m.GetElement(0, 2),
                              m.GetElement(1, 0), m.GetElement(1, 1), m.GetElement(1, 2),
                              m.GetElement(2, 0), m.GetElement(2, 1), m.GetElement(2, 2))

mapper = vtkImageSliceMapper()
mapper.SetInputData(inputImage)

# Using SINGLE_PRECISION will fail the test due to precision issues
mapper.SetOutputPointsPrecision(vtkImageSliceMapper.DOUBLE_PRECISION)

# Add the actors to the renderer, set the background and size
actor = vtkImageActor()
actor.SetMapper(mapper)
ren1.AddActor(actor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(400,400)
ren1.GetActiveCamera().SetFocalPoint(inputImage.GetCenter())
ren1.ResetCamera()

# render the image
renWin.Render()
# --- end of script --
