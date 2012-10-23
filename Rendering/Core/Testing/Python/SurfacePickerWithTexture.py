#!/usr/bin/env python

#
# Do picking with an actor with a texture.
# This example draws a cone at the pick point, with the color
# of the cone set from the color of the texture at the pick position.
#
# renderer and interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read the volume
reader = vtk.vtkJPEGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/beach.jpg")
#---------------------------------------------------------
# Do the surface rendering
sphereSource = vtk.vtkSphereSource()
sphereSource.SetRadius(100)
textureSphere = vtk.vtkTextureMapToSphere()
textureSphere.SetInputConnection(sphereSource.GetOutputPort())
sphereStripper = vtk.vtkStripper()
sphereStripper.SetInputConnection(textureSphere.GetOutputPort())
sphereStripper.SetMaximumLength(5)
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphereStripper.GetOutputPort())
sphereMapper.ScalarVisibilityOff()
sphereTexture = vtk.vtkTexture()
sphereTexture.SetInputConnection(reader.GetOutputPort())
sphereProperty = vtk.vtkProperty()
sphereProperty.BackfaceCullingOn()
sphere = vtk.vtkActor()
sphere.SetMapper(sphereMapper)
sphere.SetTexture(sphereTexture)
sphere.SetProperty(sphereProperty)
#---------------------------------------------------------
ren.AddViewProp(sphere)
camera = ren.GetActiveCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(100,400,-100)
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
picker.PickTextureDataOn()
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
picker.Pick(104,154,0,ren)
#puts [picker Print]
p = picker.GetPickPosition()
n = picker.GetPickNormal()
ijk = picker.GetPointIJK()
data = picker.GetDataSet()
i = lindex(ijk,0)
j = lindex(ijk,1)
k = lindex(ijk,2)
if (data.IsA("vtkImageData")):
    r = data.GetScalarComponentAsDouble(i,j,k,0)
    g = data.GetScalarComponentAsDouble(i,j,k,1)
    b = data.GetScalarComponentAsDouble(i,j,k,2)
    pass
else:
    r = 255.0
    g = 0.0
    b = 0.0
    pass
r = expr.expr(globals(), locals(),["r","/","255.0"])
g = expr.expr(globals(), locals(),["g","/","255.0"])
b = expr.expr(globals(), locals(),["b","/","255.0"])
coneActor1 = vtk.vtkActor()
coneActor1.PickableOff()
coneMapper1 = vtk.vtkDataSetMapper()
coneMapper1.SetInputConnection(coneSource.GetOutputPort())
coneActor1.SetMapper(coneMapper1)
coneActor1.GetProperty().SetColor(r,g,b)
coneActor1.GetProperty().BackfaceCullingOn()
coneActor1.SetPosition(lindex(p,0),lindex(p,1),lindex(p,2))
PointCone(coneActor1,lindex(n,0),lindex(n,1),lindex(n,2))
ren.AddViewProp(coneActor1)
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
