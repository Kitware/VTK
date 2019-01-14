#!/usr/bin/env python

# This script tests the vtkDistanceToCamera algorithm
# The left renderer (dark background) shows polydata
# colored homogeneously.
# The right renderer (light background) shows unstructured
# grid colored inhomogeneously as defined by the 'values'
# array.
# Zooming in and out in each renderer will change the color
# of the hexahedron.
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

pts = vtk.vtkPoints()
values = vtk.vtkFloatArray()
values.SetName('values')
for z in [0,1]:
  for y in [0,1]:
    for x in [0,1]:
      pid = pts.InsertNextPoint(x*2,y*3,z*4)

pointdata = [-1,-2,-2,-1,1,2,2,1]
for v in pointdata:
  values.InsertNextValue( v )

linecells = [[0,1],[1,3],[3,2],[2,0],[4,5],[5,7],[7,6],[6,4],[0,4],[1,5],[2,6],[3,7]]
quadcells = [[0,1,3,2],[4,5,7,6],[0,1,5,4],[1,3,7,5],[3,2,6,7],[0,2,6,4]]
hexacells = [[0,1,3,2,4,5,7,6]]

# Set up the polydata based pipeline
poly = vtk.vtkPolyData()
poly.SetPoints(pts)
poly.GetPointData().SetScalars(values)
verts = vtk.vtkCellArray()
for cell in quadcells:
  verts.InsertNextCell(len(cell))
  for p in cell:
    verts.InsertCellPoint(p)
#poly.SetVerts(verts)
lines = vtk.vtkCellArray()
for cell in linecells:
  lines.InsertNextCell(len(cell))
  for p in cell:
    lines.InsertCellPoint(p)
#poly.SetLines(lines)
quads = vtk.vtkCellArray()
for cell in quadcells:
  quads.InsertNextCell(len(cell))
  for p in cell:
    quads.InsertCellPoint(p)
poly.SetPolys(quads)
polyD2C = vtk.vtkDistanceToCamera()
polyD2C.SetInputData(poly)
polyMapper = vtk.vtkPolyDataMapper()
polyMapper.SetInputConnection(polyD2C.GetOutputPort())
polyMapper.SetScalarModeToUsePointFieldData()
polyMapper.SelectColorArray('DistanceToCamera')
polyActor = vtk.vtkActor()
polyActor.SetMapper(polyMapper)
polyRenderer = vtk.vtkRenderer()
polyRenderer.AddViewProp(polyActor)
polyRenderer.SetBackground(.2,.2,.2)
polyRenderer.SetViewport(0.,0.,0.5,1.0)
polyD2C.SetRenderer(polyRenderer)
polyRenderer.GetActiveCamera().SetFocalPoint(1,1.5,2)
polyRenderer.GetActiveCamera().SetPosition(-2,-4,6)
polyRenderer.ResetCamera()

# Set up the unstructured grid based pipeline
ug = vtk.vtkUnstructuredGrid()
ug.SetPoints(pts)
for cell in hexacells:
  ug.InsertNextCell(vtk.VTK_HEXAHEDRON,len(cell),cell)
ug.GetPointData().SetScalars(values)
ugD2C = vtk.vtkDistanceToCamera()
ugD2C.SetInputData(ug)
ugD2C.SetDistanceArrayName('d2c')
ugD2C.ScalingOn()
ugD2C.SetInputArrayToProcess(0,0,0,0,'values')
ugMapper = vtk.vtkDataSetMapper()
ugMapper.SetInputConnection(ugD2C.GetOutputPort())
ugMapper.SetScalarModeToUsePointFieldData()
ugMapper.SelectColorArray('d2c')
ugActor = vtk.vtkActor()
ugActor.SetMapper(ugMapper)
ugRenderer = vtk.vtkRenderer()
ugRenderer.AddViewProp(ugActor)
ugRenderer.SetBackground(.4,.4,.4)
ugRenderer.SetViewport(0.5,0.,1.0,1.0)
ugD2C.SetRenderer(ugRenderer)
ugRenderer.GetActiveCamera().SetFocalPoint(1,1.5,2)
ugRenderer.GetActiveCamera().SetPosition(-2,-4,6)
ugRenderer.ResetCamera()

# Render both
renWin = vtk.vtkRenderWindow()
renWin.SetSize(600,300)
renWin.AddRenderer(polyRenderer)
renWin.AddRenderer(ugRenderer)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Test zoomed out
ugRenderer.GetActiveCamera().Zoom(0.3)
polyRenderer.GetActiveCamera().Zoom(0.3)
iren.Render()

#iren.Start()
# --- end of script --
