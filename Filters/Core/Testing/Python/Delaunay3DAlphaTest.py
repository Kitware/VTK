from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersCore import vtkDelaunay3D
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

# Data from our friends at Sandia
points = vtkPoints()
points.InsertNextPoint(0,0,0)
points.InsertNextPoint(1,0,0)
points.InsertNextPoint(1,1,0)
points.InsertNextPoint(0,1,0)
points.InsertNextPoint(0,0,5)
points.InsertNextPoint(1,0,4)
points.InsertNextPoint(1,1,4)
points.InsertNextPoint(0,1,5)
points.InsertNextPoint(5,0,7)
points.InsertNextPoint(5,0,6)
points.InsertNextPoint(5,1,6)
points.InsertNextPoint(5,1,7)
points.InsertNextPoint(11,1,5)
points.InsertNextPoint(10,1,4)
points.InsertNextPoint(10,0,4)
points.InsertNextPoint(11,0,5)
points.InsertNextPoint(10,0,0)
points.InsertNextPoint(11,0,0)
points.InsertNextPoint(11,1,0)
points.InsertNextPoint(10,1,0)

profile = vtkPolyData()
profile.SetPoints(points)

# triangulate them
#
del1 = vtkDelaunay3D()
del1.SetInputData(profile)
del1.SetTolerance(0.01)
del1.SetAlpha(2.8)
del1.AlphaTetsOn()
del1.AlphaTrisOn()
del1.AlphaLinesOff()
del1.AlphaVertsOn()

map = vtkDataSetMapper()
map.SetInputConnection(del1.GetOutputPort())
triangulation = vtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1,0,0)
# Create graphics stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1,1,1)
renWin.SetSize(250,250)
cam1 = ren1.GetActiveCamera()
cam1.SetFocalPoint(0,0,0)
cam1.SetPosition(1,1,1)
ren1.ResetCamera()

# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
