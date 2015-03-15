
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Slat points into a cube
math = vtk.vtkMath()
points = vtk.vtkPoints()
i = 0
while i < 100000:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),math.Random(0,1))
    i = i + 1

profile = vtk.vtkPolyData()
profile.SetPoints(points)

# Checkerboard
cbdSplatter = vtk.vtkCheckerboardSplatter()
cbdSplatter.SetInputData(profile)
cbdSplatter.SetSampleDimensions(100, 100, 100)
cbdSplatter.ScalarWarpingOff()
cbdSplatter.SetFootprint(2)
cbdSplatter.SetParallelSplatCrossover(2)
#cbdSplatter.SetRadius(0.05)

cbdSurface = vtk.vtkMarchingContourFilter()
cbdSurface.SetInputConnection(cbdSplatter.GetOutputPort())
cbdSurface.SetValue(0, 0.01)

cbdMapper = vtk.vtkPolyDataMapper()
cbdMapper.SetInputConnection(cbdSurface.GetOutputPort())
cbdMapper.ScalarVisibilityOff()

cbdActor = vtk.vtkActor()
cbdActor.SetMapper(cbdMapper)
cbdActor.GetProperty().SetColor(1.0, 0.0, 0.0)

timer = vtk.vtkExecutionTimer()
timer.SetFilter(cbdSplatter)
cbdSplatter.Update()
CBDwallClock = timer.GetElapsedWallClockTime()
#print ("CBDSplat:", CBDwallClock)

# Graphics stuff
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)

camera = vtk.vtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(1,1,1)
ren.SetActiveCamera(camera)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(cbdActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(400, 400)
ren.ResetCamera()

# render and interact with data
iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);
renWin.Render()

#iRen.Start()

# prevent the tk window from showing up then start the event loop
# --- end of script --
