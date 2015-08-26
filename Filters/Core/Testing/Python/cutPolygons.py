import vtk
from math import sqrt
from vtk.test import Testing

plane = vtk.vtkPlane()
plane.SetOrigin(5,5,9.8)
plane.SetNormal(0,0,1)

coords = [(0,0,0),(10,0,0),(10,10,0),(0,10,0),
          (0,0,10),(10,0,10),(10,10,10),(0,10,10),
          (5,0,0),(10,5,0),(5,10,0),(0,5,0),
          (5,0,9.5),(10,5,9.3),(5,10,9.5),(0,5,9.3),
          (0,0,5),(10,0,5),(10,10,5),(0,10,5)]
data = vtk.vtkFloatArray()
points = vtk.vtkPoints()
ptIds = vtk.vtkIdList()
mesh = vtk.vtkUnstructuredGrid()
mesh.SetPoints(points)
mesh.GetPointData().SetScalars(data)

for id in range(0,20):
    x=coords[id][0]
    y=coords[id][1]
    z=coords[id][2]
    ptIds.InsertNextId(id)
    points.InsertNextPoint(x,y,z)
    data.InsertNextValue(sqrt(x*x + y*y + z*z))
mesh.InsertNextCell(vtk.VTK_QUADRATIC_HEXAHEDRON,ptIds)

ptIds.Reset()
for id in range(0,8):
    x=coords[id][0] + 20
    y=coords[id][1] + 20
    z=coords[id][2]
    ptIds.InsertNextId(id + 20)
    points.InsertNextPoint(x,y,z)
    data.InsertNextValue(sqrt(x*x + y*y + z*z))
mesh.InsertNextCell(vtk.VTK_HEXAHEDRON,ptIds)

print( "Mesh bounding box : ({0})".format( mesh.GetPoints().GetBounds() ) )

triCutter = vtk.vtkCutter()
triCutter.SetInputData(mesh)
triCutter.SetCutFunction(plane)
triCutter.GenerateTrianglesOn()
triCutter.Update()
print( "Triangle cutter bounding box : ({0})".format( triCutter.GetOutput().GetPoints().GetBounds() ) )

polyCutter = vtk.vtkCutter()
polyCutter.SetInputData(mesh)
polyCutter.SetCutFunction(plane)
polyCutter.GenerateTrianglesOff()
polyCutter.Update()
print( "Polygon cutter bounding box : ({0})".format( polyCutter.GetOutput().GetPoints().GetBounds() ) )

# Display input and output side-by-side
meshMapper = vtk.vtkDataSetMapper()
meshMapper.SetInputData(mesh)

meshActor=vtk.vtkActor()
meshActor.SetMapper(meshMapper)

meshRen = vtk.vtkRenderer()
meshRen.AddActor(meshActor)
meshRen.SetViewport(0.0,0.0,0.5,1.0)

triCutMapper = vtk.vtkPolyDataMapper()
triCutMapper.SetInputData(triCutter.GetOutput())

triCutActor=vtk.vtkActor()
triCutActor.SetMapper(triCutMapper)
triCutActor.GetProperty().EdgeVisibilityOn()
triCutActor.GetProperty().SetEdgeColor(1,1,1)

triCutRen = vtk.vtkRenderer()
triCutRen.AddActor(triCutActor)
triCutRen.SetViewport(0.5,0.5,1.0,1.0)

polyCutMapper = vtk.vtkPolyDataMapper()
polyCutMapper.SetInputData(polyCutter.GetOutput())

polyCutActor=vtk.vtkActor()
polyCutActor.SetMapper(polyCutMapper)
polyCutActor.GetProperty().EdgeVisibilityOn()
polyCutActor.GetProperty().SetEdgeColor(1,1,1)

polyCutRen = vtk.vtkRenderer()
polyCutRen.AddActor(polyCutActor)
polyCutRen.SetViewport(0.5,0.0,1.0,0.5)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(meshRen)
renWin.AddRenderer(triCutRen)
renWin.AddRenderer(polyCutRen)
renWin.SetSize(800,400)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renWin.Render()
iren.Initialize()
