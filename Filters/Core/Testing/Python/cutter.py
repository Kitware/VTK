from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import vtkCutter
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader
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
from vtkmodules.util.misc import vtkGetDataRoot

ren1 = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

xmlReader = vtkXMLUnstructuredGridReader()
xmlReader.SetFileName( vtkGetDataRoot() + '/Data/cuttertest.vtu' )

plane = vtkPlane()
plane.SetOrigin( 50,0,405 )
plane.SetNormal( 0,0,1 )

# pipeline for cutter producing triangles
triCutter = vtkCutter()
triCutter.SetInputConnection( xmlReader.GetOutputPort() )
triCutter.SetCutFunction( plane )

triMapper = vtkPolyDataMapper()
triMapper.SetInputConnection( triCutter.GetOutputPort() )
triMapper.ScalarVisibilityOff()

triActor  = vtkActor()
triActor.SetMapper( triMapper )
triActor.GetProperty().SetColor( 1,0,0 )
triActor.GetProperty().EdgeVisibilityOn()
triActor.GetProperty().SetEdgeColor( 1,1,1 )

ren1.AddViewProp( triActor )
ren1.SetViewport( 0,0,0.5,1.0)

# pipeline for cutter producing polygons
polyCutter = vtkCutter()
polyCutter.GenerateTrianglesOff()
polyCutter.SetInputConnection( xmlReader.GetOutputPort() )
polyCutter.SetCutFunction( plane )

polyMapper = vtkPolyDataMapper()
polyMapper.SetInputConnection( polyCutter.GetOutputPort() )
polyMapper.ScalarVisibilityOff()

polyActor  = vtkActor()
polyActor.SetMapper( polyMapper )
polyActor.GetProperty().SetColor( 0,0,1 )

ren2.AddViewProp( polyActor )
ren2.SetViewport( 0.5,0,1.0,1.0 )

# the render window
renWin.SetSize(600,500)
iren.Initialize()
