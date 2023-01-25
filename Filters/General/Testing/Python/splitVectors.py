#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkAssignAttribute,
    vtkContourFilter,
    vtkMergeFields,
    vtkPolyDataNormals,
    vtkStructuredGridOutlineFilter,
)
from vtkmodules.vtkFiltersFlowPaths import vtkStreamTracer
from vtkmodules.vtkFiltersGeneral import vtkSplitField
from vtkmodules.vtkFiltersModeling import vtkRibbonFilter
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
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
# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)

sf = vtkSplitField()
sf.SetInputData(output)
sf.SetInputField("VECTORS","POINT_DATA")
sf.Split(0,"vx")
sf.Split(1,"vy")
sf.Split(2,"vz")
#sf.Print()
aax = vtkAssignAttribute()
aax.SetInputConnection(sf.GetOutputPort())
aax.Assign("vx","SCALARS","POINT_DATA")
isoVx = vtkContourFilter()
isoVx.SetInputConnection(aax.GetOutputPort())
isoVx.SetValue(0,.38)
normalsVx = vtkPolyDataNormals()
normalsVx.SetInputConnection(isoVx.GetOutputPort())
normalsVx.SetFeatureAngle(45)
isoVxMapper = vtkPolyDataMapper()
isoVxMapper.SetInputConnection(normalsVx.GetOutputPort())
isoVxMapper.ScalarVisibilityOff()
isoVxActor = vtkActor()
isoVxActor.SetMapper(isoVxMapper)
isoVxActor.GetProperty().SetColor(1,0.7,0.6)

aay = vtkAssignAttribute()
aay.SetInputConnection(sf.GetOutputPort())
aay.Assign("vy","SCALARS","POINT_DATA")
isoVy = vtkContourFilter()
isoVy.SetInputConnection(aay.GetOutputPort())
isoVy.SetValue(0,.38)
normalsVy = vtkPolyDataNormals()
normalsVy.SetInputConnection(isoVy.GetOutputPort())
normalsVy.SetFeatureAngle(45)
isoVyMapper = vtkPolyDataMapper()
isoVyMapper.SetInputConnection(normalsVy.GetOutputPort())
isoVyMapper.ScalarVisibilityOff()
isoVyActor = vtkActor()
isoVyActor.SetMapper(isoVyMapper)
isoVyActor.GetProperty().SetColor(0.7,1,0.6)

aaz = vtkAssignAttribute()
aaz.SetInputConnection(sf.GetOutputPort())
aaz.Assign("vz","SCALARS","POINT_DATA")
isoVz = vtkContourFilter()
isoVz.SetInputConnection(aaz.GetOutputPort())
isoVz.SetValue(0,.38)
normalsVz = vtkPolyDataNormals()
normalsVz.SetInputConnection(isoVz.GetOutputPort())
normalsVz.SetFeatureAngle(45)
isoVzMapper = vtkPolyDataMapper()
isoVzMapper.SetInputConnection(normalsVz.GetOutputPort())
isoVzMapper.ScalarVisibilityOff()
isoVzActor = vtkActor()
isoVzActor.SetMapper(isoVzMapper)
isoVzActor.GetProperty().SetColor(0.4,0.5,1)

mf = vtkMergeFields()
mf.SetInputConnection(sf.GetOutputPort())
mf.SetOutputField("merged","POINT_DATA")
mf.SetNumberOfComponents(3)
mf.Merge(0,"vy",0)
mf.Merge(1,"vz",0)
mf.Merge(2,"vx",0)
#mf.Print()
aa = vtkAssignAttribute()
aa.SetInputConnection(mf.GetOutputPort())
aa.Assign("merged","SCALARS","POINT_DATA")
aa2 = vtkAssignAttribute()
aa2.SetInputConnection(aa.GetOutputPort())
aa2.Assign("SCALARS","VECTORS","POINT_DATA")
sl = vtkStreamTracer()
sl.SetInputConnection(aa2.GetOutputPort())
sl.SetStartPosition(2,-2,26)
sl.SetMaximumPropagation(40)
sl.SetInitialIntegrationStep(0.2)
sl.SetIntegrationDirectionToForward()
rf = vtkRibbonFilter()
rf.SetInputConnection(sl.GetOutputPort())
rf.SetWidth(1.0)
rf.SetWidthFactor(5)
slMapper = vtkPolyDataMapper()
slMapper.SetInputConnection(rf.GetOutputPort())
slActor = vtkActor()
slActor.SetMapper(slMapper)

outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(isoVxActor)
isoVxActor.AddPosition(0,12,0)
ren1.AddActor(isoVyActor)
ren1.AddActor(isoVzActor)
isoVzActor.AddPosition(0,-12,0)
ren1.AddActor(slActor)
slActor.AddPosition(0,24,0)
ren1.AddActor(outlineActor)
outlineActor.AddPosition(0,24,0)
ren1.SetBackground(.8,.8,.8)
renWin.SetSize(320,320)

ren1.GetActiveCamera().SetPosition(-20.3093,20.55444,64.3922)
ren1.GetActiveCamera().SetFocalPoint(8.255,0.0499763,29.7631)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0,0,1)
ren1.GetActiveCamera().Dolly(0.4)
ren1.ResetCameraClippingRange()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
