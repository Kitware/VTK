from vtkmodules.vtkFiltersCore import (
    vtkCellCenters,
    vtkGlyph3D,
    vtkPolyDataNormals,
)
from vtkmodules.vtkFiltersSources import (
    vtkArrowSource,
    vtkPlatonicSolidSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

renderers = []

colors = [
    (1.0, 0.3, 0.3),
    (0.9, 0.9, 0.0),
    (0.0, 1.0, 0.0),
    (0.1, 0.9, 0.9),
    (0.2, 0.4, 1.0),
]

for i in range(5):

    source = vtkPlatonicSolidSource()
    source.SetSolidType(i)
    source.Update()

    normals = vtkPolyDataNormals()
    normals.SetInputConnection(source.GetOutputPort())
    normals.ComputeCellNormalsOn()
    normals.ComputePointNormalsOff()
    normals.SplittingOff()
    normals.Update()

    centers = vtkCellCenters()
    centers.SetInputConnection(normals.GetOutputPort())
    centers.Update()

    arrow = vtkArrowSource()

    glyphs = vtkGlyph3D()
    glyphs.SetInputConnection(centers.GetOutputPort())
    glyphs.SetSourceConnection(arrow.GetOutputPort())
    glyphs.SetScaleModeToScaleByVector()
    glyphs.SetScaleFactor(0.5)
    glyphs.SetVectorModeToUseNormal()
    glyphs.Update()

    mapper = vtkPolyDataMapper()
    mapper.SetInputConnection(glyphs.GetOutputPort())
    mapper.ScalarVisibilityOff()
    actor = vtkActor()
    actor.SetMapper(mapper)

    mapper2 = vtkPolyDataMapper()
    mapper2.SetInputConnection(source.GetOutputPort())
    mapper2.ScalarVisibilityOff()
    actor2 = vtkActor()
    actor2.GetProperty().SetColor(colors[i])
    actor2.SetMapper(mapper2)

    renderer = vtkRenderer()
    renderer.SetBackground(0.0, 0.0, 0.3)
    renderer.AddActor(actor)
    renderer.AddActor(actor2)

    camera = renderer.GetActiveCamera()
    camera.SetFocalPoint(0.0, 0.0, 0.0)
    camera.SetPosition(3.0, 2.0, 8.0)
    camera.OrthogonalizeViewUp()

    renderers.append(renderer)

renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)

n = len(renderers)
for i,renderer in enumerate(renderers):
    renderer.SetViewport(i/n, 0.0, (i + 1)/n, 1.0)
    renWin.AddRenderer(renderer)
    renWin.SetSize(n*200,300)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Start()
