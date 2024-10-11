



from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkIOLegacy import vtkUnstructuredGridReader
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
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Contour the quadratic tet type
class contourQuadraticTetra(vtkmodules.test.Testing.vtkTest):

    def testContourQuadraticTetra(self):
        # Create a reader to load the data (quadratic tetrahedra)
        reader = vtkUnstructuredGridReader()
        reader.SetFileName(VTK_DATA_ROOT + "/Data/quadTetEdgeTest.vtk")

        tetContours = vtkContourFilter()
        tetContours.SetInputConnection(reader.GetOutputPort())
        tetContours.SetValue(0, 0.5)
        aTetContourMapper = vtkDataSetMapper()
        aTetContourMapper.SetInputConnection(tetContours.GetOutputPort())
        aTetContourMapper.ScalarVisibilityOff()
        aTetMapper = vtkDataSetMapper()
        aTetMapper.SetInputConnection(reader.GetOutputPort())
        aTetMapper.ScalarVisibilityOff()
        aTetActor = vtkActor()
        aTetActor.SetMapper(aTetMapper)
        aTetActor.GetProperty().SetRepresentationToWireframe()
        aTetActor.GetProperty().SetAmbient(1.0)
        aTetContourActor = vtkActor()
        aTetContourActor.SetMapper(aTetContourMapper)
        aTetContourActor.GetProperty().SetAmbient(1.0)

        # Create the rendering related stuff.
        # Since some of our actors are a single vertex, we need to remove all
        # cullers so the single vertex actors will render
        ren1 = vtkRenderer()
        ren1.GetCullers().RemoveAllItems()
        renWin = vtkRenderWindow()
        renWin.SetMultiSamples(0) # when rendering lines ensure same output since anti-aliasing is not supported on all machines
        renWin.AddRenderer(ren1)
        iren = vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        ren1.SetBackground(.1, .2, .3)

        renWin.SetSize(400, 250)

        # specify properties
        ren1.AddActor(aTetActor)
        ren1.AddActor(aTetContourActor)

        ren1.ResetCamera()
        ren1.GetActiveCamera().Dolly(1.5)
        ren1.ResetCameraClippingRange()

        renWin.Render()

        img_file = "contourQuadraticTetra.png"
        vtkmodules.test.Testing.compareImage(iren.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(contourQuadraticTetra, 'test')])
