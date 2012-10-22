#!/usr/bin/env python

# read in a Chaco file
chReader = vtk.vtkChacoReader()
chReader.SetBaseName("" + str(VTK_DATA_ROOT) + "/Data/vwgt")
chReader.SetGenerateGlobalElementIdArray(1)
chReader.SetGenerateGlobalNodeIdArray(1)
chReader.SetGenerateEdgeWeightArrays(1)
chReader.SetGenerateVertexWeightArrays(1)
geom = vtk.vtkGeometryFilter()
geom.SetInputConnection(chReader.GetOutputPort())
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(geom.GetOutputPort())
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray("VertexWeight1")
mapper.SetScalarRange(1,5)
actor0 = vtk.vtkActor()
actor0.SetMapper(mapper)
# Create the RenderWindow, Renderer and interactor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actor to the renderer, set the background and size
#
ren1.AddActor(actor0)
ren1.SetBackground(0,0,0)
renWin.SetSize(300,300)
iren.Initialize()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
