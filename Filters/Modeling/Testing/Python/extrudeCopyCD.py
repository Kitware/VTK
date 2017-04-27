#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

disk = vtk.vtkVectorText()
disk.SetText("o")
t = vtk.vtkTransform()
t.Translate(1.1,0,0)
tf = vtk.vtkTransformFilter()
tf.SetTransform(t)
tf.SetInputConnection(disk.GetOutputPort())
strips = vtk.vtkStripper()
strips.SetInputConnection(tf.GetOutputPort())
strips.Update()
app = vtk.vtkAppendPolyData()
app.AddInputData(disk.GetOutput())
app.AddInputData(strips.GetOutput())
app.Update()
model = app.GetOutput()
extrude = vtk.vtkLinearExtrusionFilter()
extrude.SetInputData(model)
# create random cell scalars for the model before extrusion.
rn = vtk.vtkMath()
rn.RandomSeed(1230)
cellColors = vtk.vtkUnsignedCharArray()
cellColors.SetNumberOfComponents(3)
cellColors.SetNumberOfTuples(model.GetNumberOfCells())
i = 0
while i < model.GetNumberOfCells():
    cellColors.InsertComponent(i,0,rn.Random(100,255))
    cellColors.InsertComponent(i,1,rn.Random(100,255))
    cellColors.InsertComponent(i,2,rn.Random(100,255))
    i = i + 1

model.GetCellData().SetScalars(cellColors)
# Lets test the arrow source instead of creating another test.
arrow1 = vtk.vtkArrowSource()
mapper1 = vtk.vtkPolyDataMapper()
mapper1.SetInputConnection(arrow1.GetOutputPort())
actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)
actor1.SetPosition(0,-0.2,1)
arrow2 = vtk.vtkArrowSource()
arrow2.SetShaftResolution(2)
arrow2.SetTipResolution(1)
mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(arrow2.GetOutputPort())
actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(1,-0.2,1)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(extrude.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(actor1)
ren1.AddActor(actor2)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# render the image
#
cam1 = ren1.GetActiveCamera()
cam1.Azimuth(20)
cam1.Elevation(40)
ren1.ResetCamera()
cam1.Zoom(1.5)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
