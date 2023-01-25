import vtk
from vtk.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(600, 200)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Read input dataset that has n-faced polyhedra
reader = vtk.vtkXMLUnstructuredGridReader()
reader.SetFileName(str(VTK_DATA_ROOT) + "/Data/poly_lines.vtu")
reader.Update()
dataset = reader.GetOutput()

# clip the dataset
clipper = vtk.vtkClipDataSet()
clipper.SetInputData(dataset)
plane = vtk.vtkPlane()
plane.SetNormal(1, 0, 0)
plane.SetOrigin(0, 1.6149157704255361, -0.98122887884924)
clipper.SetClipFunction(plane)
clipper.Update()
print("Number of cells: ", clipper.GetOutput().GetNumberOfCells())
if clipper.GetOutput().GetNumberOfCells() != 7:
    raise RuntimeError("Wrong number of cells")

# get surface representation to render
surfaceFilter = vtk.vtkDataSetSurfaceFilter()
surfaceFilter.SetInputData(clipper.GetOutput())
surfaceFilter.Update()
surface = surfaceFilter.GetOutput()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputData(surfaceFilter.GetOutput())
mapper.Update()

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToSurface()
actor.GetProperty().EdgeVisibilityOn()

ren.AddActor(actor)

ren.ResetCamera()
renWin.Render()
iren.Start()
