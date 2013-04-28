#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

#
# Demonstrate the use of clipping and capping on polyhedral data. Also shows how to
# use triangle filter to triangulate loops.
#

# create pipeline
#
# Read the polygonal data and generate vertex normals
cow = vtk.vtkBYUReader()
cow.SetGeometryFileName(VTK_DATA_ROOT + "/Data/Viewpoint/cow.g")
cowNormals = vtk.vtkPolyDataNormals()
cowNormals.SetInputConnection(cow.GetOutputPort())

# Define a clip plane to clip the cow in half
plane = vtk.vtkPlane()
plane.SetOrigin(0.25, 0, 0)
plane.SetNormal(-1, -1, 0)

clipper = vtk.vtkClipPolyData()
clipper.SetInputConnection(cowNormals.GetOutputPort())
clipper.SetClipFunction(plane)
clipper.GenerateClipScalarsOn()
clipper.GenerateClippedOutputOn()
clipper.SetValue(0.5)

clipMapper = vtk.vtkPolyDataMapper()
clipMapper.SetInputConnection(clipper.GetOutputPort())
clipMapper.ScalarVisibilityOff()

backProp = vtk.vtkProperty()
backProp.SetDiffuseColor(GetRGBColor('tomato'))

clipActor = vtk.vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(GetRGBColor('peacock'))
clipActor.SetBackfaceProperty(backProp)

# Create polygons outlining clipped areas and triangulate them to generate cut surface
cutEdges = vtk.vtkCutter()
# Generate cut lines
cutEdges.SetInputConnection(cowNormals.GetOutputPort())
cutEdges.SetCutFunction(plane)
cutEdges.GenerateCutScalarsOn()
cutEdges.SetValue(0, 0.5)
cutStrips = vtk.vtkStripper()

# Forms loops (closed polylines) from cutter
cutStrips.SetInputConnection(cutEdges.GetOutputPort())
cutStrips.Update()

cutPoly = vtk.vtkPolyData()
# This trick defines polygons as polyline loop
cutPoly.SetPoints(cutStrips.GetOutput().GetPoints())
cutPoly.SetPolys(cutStrips.GetOutput().GetLines())

cutTriangles = vtk.vtkTriangleFilter()
# Triangulates the polygons to create cut surface
cutTriangles.SetInputData(cutPoly)
cutMapper = vtk.vtkPolyDataMapper()
cutMapper.SetInputData(cutPoly)
cutMapper.SetInputConnection(cutTriangles.GetOutputPort())

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(GetRGBColor('peacock'))

# Create the rest of the cow in wireframe
restMapper = vtk.vtkPolyDataMapper()
restMapper.SetInputData(clipper.GetClippedOutput())
restMapper.ScalarVisibilityOff()

restActor = vtk.vtkActor()
restActor.SetMapper(restMapper)
restActor.GetProperty().SetRepresentationToWireframe()

# Create graphics stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren1.AddActor(clipActor)
ren1.AddActor(cutActor)
ren1.AddActor(restActor)
ren1.SetBackground(1, 1, 1)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()

renWin.SetSize(300, 300)

iren.Initialize()

# render the image

# Lets you move the cut plane back and forth
def Cut (v):
    clipper.SetValue(v)
    cutEdges.SetValue(0, v)
    cutStrips.Update()
    cutPoly.SetPoints(cutStrips.GetOutput().GetPoints())
    cutPoly.SetPolys(cutStrips.GetOutput().GetLines())
    cutMapper.Update()
    renWin.Render()

# iren.Start()
