#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkFiltersParallel import vtkCutMaterial
from vtkmodules.vtkImagingSources import (
    vtkImageEllipsoidSource,
    vtkImageGaussianSource,
)
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
# Lets create a data set.
data = vtkImageData()
data.SetExtent(0,31,0,31,0,31)
data.SetScalarType(10,data.GetInformation())
# First the data array:
gauss = vtkImageGaussianSource()
gauss.SetWholeExtent(0,30,0,30,0,30)
gauss.SetCenter(18,12,20)
gauss.SetMaximum(1.0)
gauss.SetStandardDeviation(10.0)
gauss.Update()
a = gauss.GetOutput().GetPointData().GetScalars()
a.SetName("Gauss")
data.GetCellData().SetScalars(a)
del gauss
# Now the material array:
ellipse = vtkImageEllipsoidSource()
ellipse.SetWholeExtent(0,30,0,30,0,30)
ellipse.SetCenter(11,12,13)
ellipse.SetRadius(5,9,13)
ellipse.SetInValue(1)
ellipse.SetOutValue(0)
ellipse.SetOutputScalarTypeToInt()
ellipse.Update()
m = ellipse.GetOutput().GetPointData().GetScalars()
m.SetName("Material")
data.GetCellData().AddArray(m)
del ellipse
cut = vtkCutMaterial()
cut.SetInputData(data)
cut.SetMaterialArrayName("Material")
cut.SetMaterial(1)
cut.SetArrayName("Gauss")
cut.SetUpVector(1,0,0)
cut.Update()
mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(cut.GetOutputPort())
mapper2.SetScalarRange(0,1)
#apper2 SetScalarModeToUseCellFieldData
#apper2 SetColorModeToMapScalars
#apper2 ColorByArrayComponent vtkDataSetAttributes.GhostArrayName() 0
actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(1.5,0,0)
ren = vtkRenderer()
ren.AddActor(actor2)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
p = cut.GetCenterPoint()
n = cut.GetNormal()
cam = ren.GetActiveCamera()
cam.SetFocalPoint(p)
cam.SetViewUp(cut.GetUpVector())
cam.SetPosition(n[0] + p[0], n[1] + p[1], n[2] + p[2])
ren.ResetCamera()
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
# --- end of script --
