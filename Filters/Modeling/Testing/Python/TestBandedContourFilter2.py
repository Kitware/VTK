import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script tests vtkBandedPolyDataContourFilter (BPDCF)

# Create 8 renderers, each showing a 4-point polydata with
# a different combinations of (poly)vertices, (poly)lines,
# triangles, quads, with a positive or negative range of scalars,
# and with cells that are oriented counter-clockwise or clockwise.

legend="""
Legend of the 8 renderers:
1 3 5 7
2 4 6 8

    cell orientation,   fill-cell,  scalars, scalar-mode
--------------------------------------------------------
1. counter-clockwise,      1 quad, positive,       index
2.         clockwise, 2 triangles, positive,       index
3. counter-clockwise,      1 quad, negative,       index
4.         clockwise,     1 strip, negative,       index
5. counter-clockwise,      1 quad, positive,       value
6.         clockwise, 2 triangles, positive,       value
7. counter-clockwise,      1 quad, negative,       value
8.         clockwise,     1 strip, negative,       value
"""

def InsertCell(cellArray,points,orientation):
    """
    Insert the cell counter-clockwise (orientation == True)
    or clockwise (orientation == False)
    """
    cellArray.InsertNextCell(len(points))
    if orientation:
       for p in points:
          cellArray.InsertCellPoint(p)
    else:
       for p in reversed(points):
          cellArray.InsertCellPoint(p)


def generatePolyData(orientation,fillWith,factor):
   """
   Generate poly-data and point-scalars
   """
   poly = vtk.vtkPolyData()
   pts = vtk.vtkPoints()
   coords=[ (0,0,0),(1,0,0),(1,1,0),(0,1,0)]
   for coord in coords:
      pts.InsertNextPoint(coord[0],coord[1],coord[2])
   poly.SetPoints(pts)

   # Vertices at all corners
   # two 1-point vertices and 1 2-point poly-vertex
   vertices = [[0],[1],[2,3]]
   verts = vtk.vtkCellArray()
   for vertex in vertices:
      InsertCell(verts,vertex,orientation)
   poly.SetVerts(verts)

   # Lines at all sides of the quad
   # two 2-point lines and 1 3-point line
   edges = [ (0,1),(1,2),(2,3,0) ]
   lines = vtk.vtkCellArray()
   for edge in edges:
      InsertCell(lines,edge,orientation)
   poly.SetLines(lines)

   # Fill with one quad, two triangles or a triangle-strip
   if fillWith=='quad':
      quad = (0,1,2,3)
      polys = vtk.vtkCellArray()
      InsertCell(polys,quad,orientation)
      poly.SetPolys(polys)
   elif fillWith=='triangles':
      triangles=[(0,1,3),(3,1,2)]
      strips = vtk.vtkCellArray()
      for triangle in triangles:
         InsertCell(strips,triangle,orientation)
      poly.SetStrips(strips)
   elif fillWith=='strip':
      strip=(0,1,3,2)
      strips = vtk.vtkCellArray()
      InsertCell(strips,strip,orientation)
      poly.SetStrips(strips)

   # Scalars for contouring
   values = [ 0.0, 0.5, 1.5, 1.0 ]
   array=vtk.vtkDoubleArray()
   for v in values:
      array.InsertNextValue(factor*v)
   poly.GetPointData().SetScalars(array)

   return poly

def contourCase(poly,mode):
   """
   Create a renderer, actor, mapper to contour the polydata
   poly : polydata to contour
   mode : scalar-mode for BPDCF
   """
   # Prepare an array of point-data scalars

   # Perform the contouring. Note that we need to set scalar
   # mode to index because line cells do not get contour values
   # even if scalar mode is set to value.
   valueRange=poly.GetPointData().GetScalars().GetRange()
   num=5
   bpdcf = vtk.vtkBandedPolyDataContourFilter()
   bpdcf.SetInputData(poly)
   bpdcf.GenerateValues( num, valueRange[0],valueRange[1] )
   bpdcf.GenerateContourEdgesOff()
   if mode == 'index':
     bpdcf.SetScalarModeToIndex()
   elif mode == 'value':
     bpdcf.SetScalarModeToValue()
   bpdcf.Update()

   # Shrink all cells somewhat so the contouring of edges can
   # be seen better
   sf = vtk.vtkShrinkFilter()
   sf.SetShrinkFactor(0.90)
   sf.SetInputConnection( bpdcf.GetOutputPort())

   # Mapper shows contour index values
   m = vtk.vtkDataSetMapper()
   m.SetInputConnection(sf.GetOutputPort())
   m.SetScalarModeToUseCellData()
   m.SetScalarRange(bpdcf.GetOutput().GetCellData().GetArray('Scalars').GetRange())

   a = vtk.vtkActor()
   a.SetMapper(m)

   return a

# four contouring cases to test
cases = [ (True,  'quad',      100.0, [0.0,0.5,0.5,1.0]),   # 1,5 : upper-left
          (False, 'triangles', 100.0, [0.0,0.0,0.5,0.5]),   # 2,6 : lower-left
          (True,  'quad',     -100.0, [0.5,0.5,1.0,1.0]),   # 3,7 : upper-right
          (False, 'strip',    -100.0, [0.5,0.0,1.0,0.5]), ] # 4,8 : lower-right

# Two sets of four renderers, left four yielding index-numbers, right four
# yielding contour-values
v0=0
dv=.5
renWin = vtk.vtkRenderWindow()
for mode in ['index','value']:
   for (orient,fill,factor,vp1) in cases:
      vp2 = [ v0 + vp1[0]*dv, vp1[1], v0 + vp1[2]*dv, vp1[3] ]
      poly=generatePolyData(orient,fill,factor)
      actor = contourCase(poly,mode)
      ren = vtk.vtkRenderer()
      ren.AddViewProp( actor )
      ren.SetViewport( vp2 )
      renWin.AddRenderer( ren )
   v0 += dv

print(legend)
renWin.SetSize(400,200)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
iren.Initialize()
