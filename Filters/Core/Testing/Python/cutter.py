import vtk
from vtk.util.misc import vtkGetDataRoot

ren1 = vtk.vtkRenderer()
ren2 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren2)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

xmlReader = vtk.vtkXMLUnstructuredGridReader()
xmlReader.SetFileName( vtkGetDataRoot() + '/Data/cuttertest.vtu' )

plane = vtk.vtkPlane()
plane.SetOrigin( 50,0,405 )
plane.SetNormal( 0,0,1 )

# pipeline for cutter producing triangles
triCutter = vtk.vtkCutter()
triCutter.SetInputConnection( xmlReader.GetOutputPort() )
triCutter.SetCutFunction( plane )

triMapper = vtk.vtkPolyDataMapper()
triMapper.SetInputConnection( triCutter.GetOutputPort() )
triMapper.ScalarVisibilityOff()

triActor  = vtk.vtkActor()
triActor.SetMapper( triMapper )
triActor.GetProperty().SetColor( 1,0,0 )
triActor.GetProperty().EdgeVisibilityOn()
triActor.GetProperty().SetEdgeColor( 1,1,1 )

ren1.AddViewProp( triActor )
ren1.SetViewport( 0,0,0.5,1.0)

# pipeline for cutter producing polygons
polyCutter = vtk.vtkCutter()
polyCutter.GenerateTrianglesOff()
polyCutter.SetInputConnection( xmlReader.GetOutputPort() )
polyCutter.SetCutFunction( plane )

polyMapper = vtk.vtkPolyDataMapper()
polyMapper.SetInputConnection( polyCutter.GetOutputPort() )
polyMapper.ScalarVisibilityOff()

polyActor  = vtk.vtkActor()
polyActor.SetMapper( polyMapper )
polyActor.GetProperty().SetColor( 0,0,1 )
polyActor.GetProperty().EdgeVisibilityOn()
polyActor.GetProperty().SetEdgeColor( 1,1,1 )

ren2.AddViewProp( polyActor )
ren2.SetViewport( 0.5,0,1.0,1.0 )

# the render window
renWin.SetSize(600,500)
iren.Initialize()
