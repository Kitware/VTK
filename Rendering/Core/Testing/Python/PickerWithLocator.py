#!/usr/bin/env python

#
# Do picking with a locator to speed things up
#
# renderer and interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read the volume
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.SetImageRange(1,93)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
v16.SetDataSpacing(3.2,3.2,1.5)
#---------------------------------------------------------
# Do the surface rendering
boneExtractor = vtk.vtkMarchingCubes()
boneExtractor.SetInputConnection(v16.GetOutputPort())
boneExtractor.SetValue(0,1150)
boneNormals = vtk.vtkPolyDataNormals()
boneNormals.SetInputConnection(boneExtractor.GetOutputPort())
boneNormals.SetFeatureAngle(60.0)
boneStripper = vtk.vtkStripper()
boneStripper.SetInputConnection(boneNormals.GetOutputPort())
boneStripper.SetMaximumLength(5)
boneLocator = vtk.vtkCellLocator()
boneLocator.SetDataSet(boneStripper.GetOutput())
boneLocator.LazyEvaluationOn()
boneMapper = vtk.vtkPolyDataMapper()
boneMapper.SetInputConnection(boneStripper.GetOutputPort())
boneMapper.ScalarVisibilityOff()
boneProperty = vtk.vtkProperty()
boneProperty.SetColor(1.0,1.0,0.9)
bone = vtk.vtkActor()
bone.SetMapper(boneMapper)
bone.SetProperty(boneProperty)
#---------------------------------------------------------
# Create an image actor
table = vtk.vtkLookupTable()
table.SetRange(0,2000)
table.SetRampToLinear()
table.SetValueRange(0,1)
table.SetHueRange(0,0)
table.SetSaturationRange(0,0)
mapToColors = vtk.vtkImageMapToColors()
mapToColors.SetInputConnection(v16.GetOutputPort())
mapToColors.SetLookupTable(table)
imageActor = vtk.vtkImageActor()
imageActor.GetMapper().SetInputConnection(mapToColors.GetOutputPort())
imageActor.SetDisplayExtent(32,32,0,63,0,92)
#---------------------------------------------------------
# make a clipping plane
cx = 100.8
cy = 100.8
cz = 69.0
boneClip = vtk.vtkPlane()
boneClip.SetNormal(0,1,0)
boneClip.SetOrigin(cx,cy,cz)
boneMapper.AddClippingPlane(boneClip)
#---------------------------------------------------------
ren.AddViewProp(bone)
ren.AddViewProp(imageActor)
camera = ren.GetActiveCamera()
camera.SetFocalPoint(cx,cy,cz)
camera.SetPosition(expr.expr(globals(), locals(),["cx","+","400"]),expr.expr(globals(), locals(),["cy","+","100"]),expr.expr(globals(), locals(),["cz","-","100"]))
camera.SetViewUp(0,0,-1)
ren.ResetCameraClippingRange()
renWin.Render()
#---------------------------------------------------------
# the cone should point along the Z axis
coneSource = vtk.vtkConeSource()
coneSource.CappingOn()
coneSource.SetHeight(12)
coneSource.SetRadius(5)
coneSource.SetResolution(31)
coneSource.SetCenter(6,0,0)
coneSource.SetDirection(-1,0,0)
#---------------------------------------------------------
picker = vtk.vtkCellPicker()
picker.SetTolerance(1e-6)
picker.AddLocator(boneLocator)
# A function to point an actor along a vector
def PointCone (actor,nx,ny,nz,__vtk__temp0=0,__vtk__temp1=0):
    if (expr.expr.((globals,())):
        actor.RotateWXYZ(180,0,1,0)
        actor.RotateWXYZ(180,expr.expr(globals(), locals(),["(","nx","-","1.0",")*","0.5"]),expr.expr(globals(), locals(),["ny","*","0.5"]),expr.expr(globals(), locals(),["nz","*","0.5"]))
        pass
    else:
        actor.RotateWXYZ(180,expr.expr(globals(), locals(),["(","nx","+","1.0",")*","0.5"]),expr.expr(globals(), locals(),["ny","*","0.5"]),expr.expr(globals(), locals(),["nz","*","0.5"]))
        pass

# Pick the actor
picker.Pick(70,120,0,ren)
#puts [picker Print]
p = picker.GetPickPosition()
n = picker.GetPickNormal()
coneActor1 = vtk.vtkActor()
coneActor1.PickableOff()
coneMapper1 = vtk.vtkDataSetMapper()
coneMapper1.SetInputConnection(coneSource.GetOutputPort())
coneActor1.SetMapper(coneMapper1)
coneActor1.GetProperty().SetColor(1,0,0)
coneActor1.SetPosition(lindex(p,0),lindex(p,1),lindex(p,2))
PointCone(coneActor1,lindex(n,0),lindex(n,1),lindex(n,2))
ren.AddViewProp(coneActor1)
# Pick the image
picker.Pick(170,220,0,ren)
#puts [picker Print]
p = picker.GetPickPosition()
n = picker.GetPickNormal()
coneActor2 = vtk.vtkActor()
coneActor2.PickableOff()
coneMapper2 = vtk.vtkDataSetMapper()
coneMapper2.SetInputConnection(coneSource.GetOutputPort())
coneActor2.SetMapper(coneMapper2)
coneActor2.GetProperty().SetColor(1,0,0)
coneActor2.SetPosition(lindex(p,0),lindex(p,1),lindex(p,2))
PointCone(coneActor2,lindex(n,0),lindex(n,1),lindex(n,2))
ren.AddViewProp(coneActor2)
# Pick the actor again, in a way that the ray gets clipped
picker.Pick(180,220,0,ren)
p = picker.GetPickPosition()
n = picker.GetPickNormal()
coneActor3 = vtk.vtkActor()
coneActor3.PickableOff()
coneMapper3 = vtk.vtkDataSetMapper()
coneMapper3.SetInputConnection(coneSource.GetOutputPort())
coneActor3.SetMapper(coneMapper3)
coneActor3.GetProperty().SetColor(1,0,0)
coneActor3.SetPosition(lindex(p,0),lindex(p,1),lindex(p,2))
PointCone(coneActor3,lindex(n,0),lindex(n,1),lindex(n,2))
ren.AddViewProp(coneActor3)
ren.ResetCameraClippingRange()
renWin.Render()
#---------------------------------------------------------
# test-related code
def TkCheckAbort (__vtk__temp0=0,__vtk__temp1=0):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver(AbortCheckEvent,TkCheckAbort)
iren.Initialize()
# --- end of script --
