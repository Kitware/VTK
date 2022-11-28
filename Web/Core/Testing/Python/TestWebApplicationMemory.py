import vtk
from vtk.test import Testing
from vtk.vtkWebCore import vtkWebApplication

class TestWebApplicationMemory(Testing.vtkTest):
    def testWebApplicationMemory(self):
        cylinder = vtk.vtkCylinderSource()
        cylinder.SetResolution(8)

        cylinderMapper = vtk.vtkPolyDataMapper()
        cylinderMapper.SetInputConnection(cylinder.GetOutputPort())

        cylinderActor = vtk.vtkActor()
        cylinderActor.SetMapper(cylinderMapper)
        cylinderActor.RotateX(30.0)
        cylinderActor.RotateY(-45.0)

        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        ren.AddActor(cylinderActor)
        renWin.SetSize(200, 200)

        ren.ResetCamera()
        ren.GetActiveCamera().Zoom(1.5)
        renWin.Render()

        webApp = vtk.vtkWebApplication()
        # no memory leaks should be reported when compiling with VTK_DEBUG_LEAKS
        webApp.StillRender(renWin)

if __name__ == "__main__":
    Testing.main([(TestWebApplicationMemory, 'test')])
