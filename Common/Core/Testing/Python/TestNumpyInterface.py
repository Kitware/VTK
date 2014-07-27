import sys

try:
    import numpy
except ImportError:
    print "Numpy (http://numpy.scipy.org) not found.",
    print "This test requires numpy!"
    sys.exit(0)

import vtk
import vtk.numpy_interface.dataset_adapter as dsa
import vtk.numpy_interface.algorithms as algs

w = vtk.vtkRTAnalyticSource()

bp = vtk.vtkBrownianPoints()
bp.SetInputConnection(w.GetOutputPort())
bp.Update()

elev = vtk.vtkElevationFilter()
elev.SetInputConnection(bp.GetOutputPort())
elev.SetLowPoint(-10, 0, 0)
elev.SetHighPoint(10, 0, 0)
elev.SetScalarRange(0, 20)

g = vtk.vtkMultiBlockDataGroupFilter()
g.AddInputConnection(elev.GetOutputPort())
g.AddInputConnection(elev.GetOutputPort())

g.Update()

elev2 = vtk.vtkElevationFilter()
elev2.SetInputConnection(bp.GetOutputPort())
elev2.SetLowPoint(0, -10, 0)
elev2.SetHighPoint(0, 10, 0)
elev2.SetScalarRange(0, 20)

g2 = vtk.vtkMultiBlockDataGroupFilter()
g2.AddInputConnection(elev2.GetOutputPort())
g2.AddInputConnection(elev2.GetOutputPort())

g2.Update()

elev3 = vtk.vtkElevationFilter()
elev3.SetInputConnection(bp.GetOutputPort())
elev3.SetLowPoint(0, 0, -10)
elev3.SetHighPoint(0, 0, 10)
elev3.SetScalarRange(0, 20)

g3 = vtk.vtkMultiBlockDataGroupFilter()
g3.AddInputConnection(elev3.GetOutputPort())
g3.AddInputConnection(elev3.GetOutputPort())

g3.Update()

cd = dsa.CompositeDataSet(g.GetOutput())
randomVec = cd.PointData['BrownianVectors']
elev = cd.PointData['Elevation']

cd2 = dsa.CompositeDataSet(g2.GetOutput())
elev2 = cd2.PointData['Elevation']

cd3 = dsa.CompositeDataSet(g3.GetOutput())
elev3 = cd3.PointData['Elevation']

npa = randomVec.Arrays[0]

# Test operators
assert algs.all(1 + randomVec - 1 - randomVec < 1E-4)

assert (1 + randomVec).DataSet is randomVec.DataSet

# Test slicing and indexing
assert algs.all(randomVec[randomVec[:,0] > 0.2].Arrays[0] - npa[npa[:,0] > 0.2] < 1E-7)
assert algs.all(randomVec[algs.where(randomVec[:,0] > 0.2)].Arrays[0] - npa[numpy.where(npa[:,0] > 0.2)] < 1E-7)
assert algs.all(randomVec[dsa.VTKCompositeDataArray([(slice(None, None, None), slice(0,2,None)), 2])].Arrays[0] - npa[:, 0:2] < 1E-6)

# Test ufunc
assert algs.all(algs.cos(randomVec) - numpy.cos(npa) < 1E-7)
assert algs.cos(randomVec).DataSet is randomVec.DataSet

# Various numerical ops implemented in VTK
g = algs.gradient(elev)
assert algs.all(g[0] == (1, 0, 0))

v = algs.make_vector(elev, g[:,0], elev)
assert algs.all(algs.gradient(v) == [[1, 0, 0], [0, 0, 0], [1, 0, 0]])

v = algs.make_vector(elev, g[:,0], elev2)
assert algs.all(algs.curl(v) == [1, 0, 0])

v = algs.make_vector(elev, elev2, 2*elev3)
g = algs.gradient(v)
assert g.DataSet is v.DataSet
assert algs.all(algs.det(g) == 2)

assert algs.all(algs.eigenvalue(g) == [2, 1, 1])

assert algs.all(randomVec[:,0] == randomVec[:,0])

ssource = vtk.vtkSphereSource()
ssource.Update()

output = ssource.GetOutput()

fd = vtk.vtkFloatArray()
fd.SetNumberOfTuples(11)
fd.FillComponent(0, 5)
fd.SetName("field array")

output.GetFieldData().AddArray(fd)

g2 = vtk.vtkMultiBlockDataGroupFilter()
g2.AddInputData(output)
g2.AddInputData(output)

g2.Update()

sphere = dsa.CompositeDataSet(g2.GetOutput())

vn = algs.vertex_normal(sphere)
assert algs.all(algs.mag(vn) - 1 < 1E-6)

sn = algs.surface_normal(sphere)
assert algs.all(algs.mag(sn) - 1 < 1E-6)

dot = algs.dot(vn, vn)
assert dot.DataSet is sphere
assert algs.all(dot == 1)
assert algs.all(algs.cross(vn, vn) == [0, 0, 0])

fd = sphere.FieldData['field array']
assert algs.all(fd == 5)
assert algs.shape(fd) == (22,)

assert vn.DataSet is sphere

# --------------------------------------

na = dsa.NoneArray

# Test operators
assert (1 + na - 1 - randomVec) is na

# Test slicing and indexing
assert na[:, 0] is na
assert algs.where(na[:, 0] > 0) is na
assert (na > 0) is na

# Test ufunc
assert algs.cos(na) is na

# Various numerical ops implemented in VTK
assert algs.gradient(na) is na
assert algs.cross(na, na) is na
assert algs.cross(v.Arrays[0], na) is na
assert algs.cross(na, v.Arrays[0]) is na

assert algs.make_vector(na, g[:,0], elev) is na

pd = vtk.vtkPolyData()
pdw = dsa.WrapDataObject(pd)
pdw.PointData.append(na, 'foo')
assert pdw.PointData.GetNumberOfArrays() == 0

# --------------------------------------

na2 = dsa.VTKCompositeDataArray([randomVec.Arrays[0], na])

# Test operators
assert (1 + na2 - 1 - randomVec).Arrays[1] is na

# Test slicing and indexing
assert na2[:, 0].Arrays[1] is na
assert algs.where(na2[:, 0] > 0).Arrays[1] is na
assert (na2 > 0).Arrays[1] is na

# Test ufunc
assert algs.cos(na2).Arrays[1] is na

# Various numerical ops implemented in VTK
assert algs.gradient(na2).Arrays[1] is na
assert algs.cross(na2, na2).Arrays[1] is na
assert algs.cross(v, na2).Arrays[1] is na
assert algs.cross(na2, v).Arrays[1] is na

assert algs.make_vector(na2[:, 0], elev, elev).Arrays[1] is na
assert algs.make_vector(elev, elev, na2[:, 0]).Arrays[1] is na
assert algs.make_vector(elev, na2[:, 0], elev).Arrays[1] is na

mb = vtk.vtkMultiBlockDataSet()
mb.SetBlock(0, pd)
pd2 = vtk.vtkPolyData()
mb.SetBlock(1, pd2)
mbw = dsa.WrapDataObject(mb)

mbw.PointData.append(dsa.NoneArray, 'foo')
assert mbw.GetBlock(0).GetPointData().GetNumberOfArrays() == 0
assert mbw.GetBlock(1).GetPointData().GetNumberOfArrays() == 0

mbw.PointData.append(na2, 'foo')
assert mbw.GetBlock(0).GetPointData().GetNumberOfArrays() == 1
assert mbw.GetBlock(1).GetPointData().GetNumberOfArrays() == 0
assert mbw.GetBlock(0).GetPointData().GetArray(0).GetName() == 'foo'

mbw.PointData.append(algs.max(na2), "maxfoo")
assert mbw.GetBlock(0).GetPointData().GetNumberOfArrays() == 2
assert mbw.GetBlock(1).GetPointData().GetNumberOfArrays() == 1
assert mbw.GetBlock(0).GetPointData().GetArray(1).GetName() == 'maxfoo'

# --------------------------------------

mb = vtk.vtkMultiBlockDataSet()
mb.SetBlock(0, vtk.vtkImageData())
mb.SetBlock(1, vtk.vtkImageData())
assert dsa.WrapDataObject(mb).Points is na

mb = vtk.vtkMultiBlockDataSet()
mb.SetBlock(0, vtk.vtkStructuredGrid())
mb.SetBlock(1, vtk.vtkImageData())
assert dsa.WrapDataObject(mb).Points is na

mb = vtk.vtkMultiBlockDataSet()
sg = vtk.vtkStructuredGrid()
sg.SetPoints(vtk.vtkPoints())
mb.SetBlock(0, sg)
mb.SetBlock(1, vtk.vtkImageData())
assert dsa.WrapDataObject(mb).Points.Arrays[0] is not na
assert dsa.WrapDataObject(mb).Points.Arrays[1] is na
