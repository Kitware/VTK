from __future__ import print_function

import os
import sys

from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
    vtkVertex,
)
from vtkmodules.vtkFiltersCore import (
    vtkElevationFilter,
    vtkExtractEdges,
    vtkGlyph3D,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkIOGeoJSON import vtkGeoJSONWriter

ss = vtkSphereSource() #make mesh to test with

af = vtkElevationFilter() #add some attributes
af.SetInputConnection(ss.GetOutputPort())

ef = vtkExtractEdges() #make lines to test
ef.SetInputConnection(af.GetOutputPort())

gf = vtkGlyph3D() #make verts to test
pts = vtkPoints()
pts.InsertNextPoint(0,0,0)
verts = vtkCellArray()
avert = vtkVertex()
avert.GetPointIds().SetId(0, 0)
verts.InsertNextCell(avert)
onevertglyph = vtkPolyData()
onevertglyph.SetPoints(pts)
onevertglyph.SetVerts(verts)
gf.SetSourceData(onevertglyph)
gf.SetInputConnection(af.GetOutputPort())

testwrites = ["points","lines","mesh"]
failed = False
for datasetString in testwrites:
  if datasetString == "points":
    toshow=gf
  elif datasetString == "lines":
    toshow = ef
  else:
    toshow = af
  gw = vtkGeoJSONWriter()
  fname = "sphere_"+datasetString+".json"
  gw.SetInputConnection(toshow.GetOutputPort())
  gw.SetFileName(fname)
  gw.Write()
  if (os.path.exists(fname) and
     os.path.isfile(fname)):
    os.remove(fname)
  else:
    print("Failed to write " + fname + " to file")
    failed = True
  gw.WriteToOutputStringOn()
  gw.Write()
  gj = "['"+str(gw.RegisterAndGetOutputString()).replace('\n','')+"']"
  if len(gj) <= 1000:
    print("Failed to write " + fname + " to buffer")
    failed = True

if failed:
  sys.exit(1)
sys.exit(0)
