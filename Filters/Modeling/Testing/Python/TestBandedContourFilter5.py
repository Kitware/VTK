#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This test reproduces an issue with vtkBandedPolyDataContourFilter
# showing anomalous spikes from edges at which the difference between
# end point scalar values is in the order of the internal tolerance
# value.
#
# The ClipEdge method interpolates between two edge points, creating
# extra points at the clip values. The method finds the iterators
# b and e into the clip values vector for the edge points.
# The clip value for the highest scalar value may be larger than the
# end point scalar value. When the difference between the end point
# values is very small this results in an interpolation factor
# significantly larger than 1.
#
# The effect of this is that spikes appear extending
# outside the original cell.

# Scalars for contouring
values = [
           -10.0, -5e-6, -1e-6,
           -10.0, -7e-6, -1e-6,
           ]

def generatePoly(scalars):
   """
   Generate two quads and assign scalars
   3--4--5
   |  |  |
   0--1--2
   """
   pts = vtk.vtkPoints()
   nx=3
   ny=int(len(scalars)/nx)
   for y in range(0,ny):
    for x in range(0,nx):
      pts.InsertNextPoint(x,y,0)

   connectivity=[(0,1,4,3),(1,2,5,4)]
   quads = vtk.vtkCellArray()
   for quad in connectivity:
       quads.InsertNextCell(4,quad)

   poly = vtk.vtkPolyData()
   poly.SetPoints(pts)
   poly.SetPolys(quads)

   array=vtk.vtkDoubleArray()
   for s in scalars:
      array.InsertNextValue(s)
   poly.GetPointData().SetScalars(array)
   return poly


def showBandedContours(poly,values,num):
   """ Generate banded contours """
   high=max(values)
   low=min(values)

   bpdcf = vtk.vtkBandedPolyDataContourFilter()
   bpdcf.GenerateContourEdgesOn()
   bpdcf.SetScalarModeToValue()
   bpdcf.SetInputData(poly)
   bpdcf.GenerateValues(num,low,high)

   # The clip tolerance specifies a fraction of the scalar range
   # It is adjusted to reproduce the issue described
   bpdcf.SetClipTolerance(1e-6)
   bpdcf.SetScalarModeToIndex()
   #internalTol=bpdcf.GetClipTolerance()*(high-low)
   #print("internal clip tolerance={}".format(internalTol))

   m = vtk.vtkPolyDataMapper()
   m.SetInputConnection(bpdcf.GetOutputPort())
   m.SetScalarModeToUseCellData()
   m.SetScalarRange(0,num-1)

   bands = vtk.vtkActor()
   bands.SetMapper(m)

   m = vtk.vtkPolyDataMapper()
   m.SetInputConnection(bpdcf.GetOutputPort(1))
   m.ScalarVisibilityOff()

   edges = vtk.vtkActor()
   edges.GetProperty().SetColor(.4,.4,.4)
   edges.SetMapper(m)

   return bands,edges

def showPolyDataEdges(poly):
   edges=vtk.vtkExtractEdges()
   edges.SetInputDataObject(0,poly)
   m = vtk.vtkPolyDataMapper()
   m.SetInputConnection(edges.GetOutputPort())
   m.ScalarVisibilityOff()
   a = vtk.vtkActor()
   a.GetProperty().SetColor(1,1,1)
   a.GetProperty().EdgeVisibilityOn()
   a.GetProperty().RenderLinesAsTubesOn()
   a.SetMapper(m)
   return a

poly=generatePoly(values)

inbounds=poly.GetBounds()

bands,edges = showBandedContours(poly,values,6)

bands.GetMapper().Update()

# The bounds of the output of vtkBandedPolyDataContourFilter
# are expected not to exceed those of its input
outbounds=bands.GetMapper().GetInputDataObject(0,0).GetBounds()
error=0
if (inbounds[0] > outbounds[0] or
    inbounds[1] < outbounds[1] or
    inbounds[2] > outbounds[2] or
    inbounds[3] < outbounds[3] or
    inbounds[4] > outbounds[4] or
    inbounds[5] < outbounds[5]):
  print("Output bounds exceed input bounds")
  print("input bounds={}".format(inbounds))
  print("output bounds={}".format(outbounds))
  error=1

ren = vtk.vtkRenderer()
ren.AddViewProp( bands )
ren.AddViewProp( edges )
ren.AddViewProp( showPolyDataEdges(poly) )
ren.SetBackground(.6,.6,.6)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

ren.GetActiveCamera().SetFocalPoint(1,.5,0)
ren.GetActiveCamera().SetPosition(1,.5,5)
ren.GetActiveCamera().SetViewUp(0,1,0)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Start()
