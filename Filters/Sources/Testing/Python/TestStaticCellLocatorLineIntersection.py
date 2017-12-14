#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# retrieve named colors
def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Control resolution of test (sphere resolution)
res = 9

# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create pipeline. Two spheres: one is the target to
# be intersected against, and is placed inside a static
# cell locator. The second bounds these, it's points
# serve as starting points that shoot rays towards the
# center of the fist sphere.
#
sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(2*res)
sphere.SetPhiResolution(res)
sphere.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Now the locator
loc = vtk.vtkStaticCellLocator()
loc.SetDataSet(sphere.GetOutput())
loc.SetNumberOfCellsPerNode(5)
loc.BuildLocator()

locPD = vtk.vtkPolyData()
loc.GenerateRepresentation(4,locPD)
locMapper = vtk.vtkPolyDataMapper()
locMapper.SetInputData(locPD)
locActor = vtk.vtkActor()
locActor.SetMapper(locMapper)
locActor.GetProperty().SetRepresentationToWireframe()

# Now the outer sphere
sphere2 = vtk.vtkSphereSource()
sphere2.SetThetaResolution(res)
sphere2.SetPhiResolution(int(res/2))
sphere2.SetRadius(3*sphere.GetRadius())
sphere2.Update()

# Generate intersection points
center = sphere.GetCenter()
polyInts = vtk.vtkPolyData()
pts = vtk.vtkPoints()
spherePts = sphere2.GetOutput().GetPoints()
numRays = spherePts.GetNumberOfPoints()
pts.SetNumberOfPoints(numRays + 1)

polyRays = vtk.vtkPolyData()
rayPts = vtk.vtkPoints()
rayPts.SetNumberOfPoints(numRays + 1)
lines = vtk.vtkCellArray()

t = vtk.reference(0.0)
subId = vtk.reference(0)
cellId = vtk.reference(0)
xyz = [0.0,0.0,0.0]
xInt = [0.0,0.0,0.0]
pc = [0.0,0.0,0.0]

pts.SetPoint(0,center)
rayPts.SetPoint(0,center)
for i in range(0, numRays):
    spherePts.GetPoint(i,xyz)
    rayPts.SetPoint(i+1,xyz)
    cellId = vtk.reference(i);
    hit = loc.IntersectWithLine(xyz, center, 0.001, t, xInt, pc, subId, cellId)
    if ( hit == 0 ):
        print("Missed: {}".format(i))
        pts.SetPoint(i+1,center)
    else:
        pts.SetPoint(i+1,xInt)
    lines.InsertNextCell(2)
    lines.InsertCellPoint(0)
    lines.InsertCellPoint(i+1)

polyInts.SetPoints(pts)

polyRays.SetPoints(rayPts)
polyRays.SetLines(lines)

# Glyph the intersection points
glyphSphere = vtk.vtkSphereSource()
glyphSphere.SetPhiResolution(6)
glyphSphere.SetThetaResolution(12)

glypher = vtk.vtkGlyph3D()
glypher.SetInputData(polyInts)
glypher.SetSourceConnection(glyphSphere.GetOutputPort())
glypher.SetScaleFactor(0.05)

glyphMapper = vtk.vtkPolyDataMapper()
glyphMapper.SetInputConnection(glypher.GetOutputPort())

glyphActor = vtk.vtkActor()
glyphActor.SetMapper(glyphMapper)
glyphActor.GetProperty().SetColor(GetRGBColor('peacock'))

linesMapper = vtk.vtkPolyDataMapper()
linesMapper.SetInputData(polyRays)

linesActor = vtk.vtkActor()
linesActor.SetMapper(linesMapper)
linesActor.GetProperty().SetColor(GetRGBColor('tomato'))

# Render it
ren.AddActor(actor)
ren.AddActor(glyphActor)
ren.AddActor(locActor)
ren.AddActor(linesActor)

ren.GetActiveCamera().SetPosition(1,1,1)
ren.GetActiveCamera().SetFocalPoint(0,0,0)
ren.ResetCamera()

renWin.Render()
iren.Start()
