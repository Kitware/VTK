#!/usr/bin/env python

renWin = vtk.vtkRenderWindow()
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renderer = vtk.vtkRenderer()
renWin.AddRenderer(renderer)
src1 = vtk.vtkSphereSource()
src1.SetRadius(5)
src1.SetPhiResolution(20)
src1.SetThetaResolution(20)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(src1.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
# Load the material. Here, we are loading a material
# defined in the Vtk Library. One can also specify
# a filename to a material description xml.
actor.GetProperty().LoadMaterial("CgTwisted")
# Turn shading on. Otherwise, shaders are not used.
actor.GetProperty().ShadingOn()
# Pass a shader variable need by CgTwisted.
actor.GetProperty().AddShaderVariable("Rate",1.0)
renderer.AddActor(actor)
renWin.Render()
renderer.GetActiveCamera().Azimuth(-50)
renderer.GetActiveCamera().Roll(70)
renWin.Render()
# --- end of script --
