from vtkmodules.vtkFiltersSources import vtkCylinderSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
from vtkmodules.vtkWebCore import vtkWebApplication
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing
from vtkmodules.vtkWebCore import vtkWebApplication

class TestWebApplicationMemory(Testing.vtkTest):
    def testWebApplicationMemory(self):
        cylinder = vtkCylinderSource()
        cylinder.SetResolution(8)

        cylinderMapper = vtkPolyDataMapper()
        cylinderMapper.SetInputConnection(cylinder.GetOutputPort())

        cylinderActor = vtkActor()
        cylinderActor.SetMapper(cylinderMapper)
        cylinderActor.RotateX(30.0)
        cylinderActor.RotateY(-45.0)

        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)
        ren.AddActor(cylinderActor)
        renWin.SetSize(200, 200)

        ren.ResetCamera()
        ren.GetActiveCamera().Zoom(1.5)
        renWin.Render()

        webApp = vtkWebApplication()
        # no memory leaks should be reported when compiling with VTK_DEBUG_LEAKS
        webApp.StillRender(renWin)

if __name__ == "__main__":
    Testing.main([(TestWebApplicationMemory, 'test')])
