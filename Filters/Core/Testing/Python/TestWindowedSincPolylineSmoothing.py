#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the render window and renderers
#
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0,0,0.5,1.0)
ren1.SetBackground(1,1,1)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5,0,1.0,1.0)
ren2.SetBackground(1,1,1)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.SetMultiSamples(0)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create synthetic, aliased data
# Create a cross and rotate it a small amount to enhance aliasing
pd = vtk.vtkPolyData()
pts = vtk.vtkPoints()
polys = vtk.vtkCellArray()

pts.SetNumberOfPoints(12)
pts.SetPoint(0, 1, -4, 0)
pts.SetPoint(1, 1, -1, 0)
pts.SetPoint(2, 4, -1, 0)
pts.SetPoint(3, 4, 1, 0)
pts.SetPoint(4, 1, 1, 0)
pts.SetPoint(5, 1, 4, 0)
pts.SetPoint(6, -1, 4, 0)
pts.SetPoint(7, -1, 1, 0)
pts.SetPoint(8, -4, 1, 0)
pts.SetPoint(9, -4, -1, 0)
pts.SetPoint(10, -1, -1, 0)
pts.SetPoint(11, -1, -4, 0)

polys.InsertNextCell(4)
polys.InsertCellPoint(1)
polys.InsertCellPoint(4)
polys.InsertCellPoint(7)
polys.InsertCellPoint(10)

polys.InsertNextCell(4)
polys.InsertCellPoint(11)
polys.InsertCellPoint(0)
polys.InsertCellPoint(1)
polys.InsertCellPoint(10)

polys.InsertNextCell(4)
polys.InsertCellPoint(1)
polys.InsertCellPoint(2)
polys.InsertCellPoint(3)
polys.InsertCellPoint(4)

polys.InsertNextCell(4)
polys.InsertCellPoint(4)
polys.InsertCellPoint(5)
polys.InsertCellPoint(6)
polys.InsertCellPoint(7)

polys.InsertNextCell(4)
polys.InsertCellPoint(7)
polys.InsertCellPoint(8)
polys.InsertCellPoint(9)
polys.InsertCellPoint(10)

pd.SetPoints(pts)
pd.SetPolys(polys)

transform = vtk.vtkTransform()
transform.RotateZ(17)
transform.Translate(0.1,0,0)

xform = vtk.vtkTransformPolyDataFilter()
xform.SetInputData(pd)
xform.SetTransform(transform)

# Rasterize through the renderer
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(xform.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

renWin.SetSize(50,50)
ren1.AddActor(actor)
renWin.Render()

renSource = vtk.vtkWindowToImageFilter()
renSource.SetInput(renWin)
renSource.Update()

# This trick decouples the pipeline so that updates
# do not affect the renSource.
output = renSource.GetOutput()
img = vtk.vtkImageData()
img.DeepCopy(output)
ren1.RemoveActor(actor)

# Now process the test image. Smooth out the aliasing.

# Next filter can only handle RGB
extract = vtk.vtkImageExtractComponents()
extract.SetInputData(img)
extract.SetComponents(0,1,2)

# Quantize the image into an index.
quantize = vtk.vtkImageQuantizeRGBToIndex()
quantize.SetInputConnection(extract.GetOutputPort())
quantize.SetNumberOfColors(3)

# Create the pipeline. This creates discrete polylines.
discrete = vtk.vtkDiscreteFlyingEdges2D()
discrete.SetInputConnection(quantize.GetOutputPort())
discrete.SetValue(0,2)

# Create polygons
polyLoops = vtk.vtkContourLoopExtraction()
polyLoops.SetInputConnection(discrete.GetOutputPort())
polyLoops.SetOutputModeToPolylines()

# Polygons are displayed
polyMapper = vtk.vtkPolyDataMapper()
polyMapper.SetInputConnection(polyLoops.GetOutputPort())

polyActor = vtk.vtkActor()
polyActor.SetMapper(polyMapper)
polyActor.GetProperty().SetColor(0,1,0)

# Now smooth
lineStrip = vtk.vtkStripper()
lineStrip.SetInputConnection(discrete.GetOutputPort())

smoother = vtk.vtkWindowedSincPolyDataFilter()
smoother.SetInputConnection(lineStrip.GetOutputPort())
smoother.SetNumberOfIterations(50)
smoother.SetEdgeAngle(90)

smoothMapper = vtk.vtkPolyDataMapper()
smoothMapper.SetInputConnection(smoother.GetOutputPort())

smoothActor = vtk.vtkActor()
smoothActor.SetMapper(smoothMapper)
smoothActor.GetProperty().SetColor(0,1,0)

ren1.AddActor(polyActor)
ren2.AddActor(smoothActor)
ren2.SetActiveCamera(ren1.GetActiveCamera())

renWin.SetSize(300,150)
ren1.ResetCamera();
renWin.Render()

iren.Start()
