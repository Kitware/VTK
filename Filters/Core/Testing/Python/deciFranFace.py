#!/usr/bin/env python

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren2 = vtk.vtkRenderer()
ren3 = vtk.vtkRenderer()
ren4 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
pnm1 = vtk.vtkPNGReader()
pnm1.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fran_cut.png")
atext = vtk.vtkTexture()
atext.SetInputConnection(pnm1.GetOutputPort())
atext.InterpolateOn()
# create a cyberware source
#
fran = vtk.vtkPolyDataReader()
fran.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fran_cut.vtk")
topologies = "On Off"
accumulates = "On Off"
for topology in topologies.split():
    for accumulate in accumulates.split():
        locals()[get_variable_name("deci", topology, "", accumulate, "")] = vtk.vtkDecimatePro()
        locals()[get_variable_name("deci", topology, "", accumulate, "")].SetInputConnection(fran.GetOutputPort())
        locals()[get_variable_name("deci", topology, "", accumulate, "")].SetTargetReduction(.95)
        locals()[get_variable_name("deci", topology, "", accumulate, "")].locals()[get_variable_name("PreserveTopology", topology, "")]()
        locals()[get_variable_name("deci", topology, "", accumulate, "")].locals()[get_variable_name("AccumulateError", accumulate, "")]()
        locals()[get_variable_name("mapper", topology, "", accumulate, "")] = vtk.vtkPolyDataMapper()
        locals()[get_variable_name("mapper", topology, "", accumulate, "")].SetInputConnection(locals()[get_variable_name("deci", topology, "", accumulate, "")].GetOutputPort())
        locals()[get_variable_name("fran", topology, "", accumulate, "")] = vtk.vtkActor()
        locals()[get_variable_name("fran", topology, "", accumulate, "")].SetMapper(locals()[get_variable_name("mapper", topology, "", accumulate, "")])
        locals()[get_variable_name("fran", topology, "", accumulate, "")].SetTexture(atext)

        pass

    pass
# Add the actors to the renderer, set the background and size
#
ren1.SetViewport(0,.5,.5,1)
ren2.SetViewport(.5,.5,1,1)
ren3.SetViewport(0,0,.5,.5)
ren4.SetViewport(.5,0,1,.5)
ren1.AddActor(franOnOn)
ren2.AddActor(franOnOff)
ren3.AddActor(franOffOn)
ren4.AddActor(franOffOff)
camera = vtk.vtkCamera()
ren1.SetActiveCamera(camera)
ren2.SetActiveCamera(camera)
ren3.SetActiveCamera(camera)
ren4.SetActiveCamera(camera)
ren1.GetActiveCamera().SetPosition(0.314753,-0.0699988,-0.264225)
ren1.GetActiveCamera().SetFocalPoint(0.00188636,-0.136847,-5.84226e-09)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0,1,0)
ren1.ResetCameraClippingRange()
ren2.ResetCameraClippingRange()
ren3.ResetCameraClippingRange()
ren4.ResetCameraClippingRange()
ren1.SetBackground(1,1,1)
ren2.SetBackground(1,1,1)
ren3.SetBackground(1,1,1)
ren4.SetBackground(1,1,1)
renWin.SetSize(500,500)
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
