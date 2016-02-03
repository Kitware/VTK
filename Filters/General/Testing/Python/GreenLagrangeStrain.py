#!/usr/bin/env python
#coding=utf8

from __future__ import print_function

import sys
import math
try:
    import numpy
except ImportError:
    print("WARNING: This test requires NumPy: http://http://www.numpy.org/")
    sys.exit(0)
import vtk


# Create a simple hexahedron
points = vtk.vtkPoints()
points.SetNumberOfPoints(8)
points.InsertPoint(0, 0, 0, 0)
points.InsertPoint(1, 1, 0, 0)
points.InsertPoint(2, 1, 1, 0)
points.InsertPoint(3, 0, 1, 0)
points.InsertPoint(4, 0, 0, 1)
points.InsertPoint(5, 1, 0, 1)
points.InsertPoint(6, 1, 1, 1)
points.InsertPoint(7, 0, 1, 1)
#for k_point in xrange(8): print points.GetPoint(k_point)

hexahedron = vtk.vtkHexahedron()
for k_point in xrange(8):
    hexahedron.GetPointIds().SetId(k_point, k_point)

cell_array = vtk.vtkCellArray()
cell_array.InsertNextCell(hexahedron)

farray_disp = vtk.vtkFloatArray()
farray_disp.SetName("displacement")
farray_disp.SetNumberOfComponents(3)
farray_disp.SetNumberOfTuples(8)

ugrid_hex = vtk.vtkUnstructuredGrid()
ugrid_hex.SetPoints(points)
ugrid_hex.SetCells(hexahedron.GetCellType(), cell_array)
ugrid_hex.GetPointData().AddArray(farray_disp)
ugrid_hex.GetPointData().SetActiveVectors("displacement")

# Test a few deformation gradients
F_lst  = []

# Simple extentions
F_lst += [numpy.array([[2,0,0],
                       [0,1,0],
                       [0,0,1]])]
F_lst += [numpy.array([[1,0,0],
                       [0,2,0],
                       [0,0,1]])]
F_lst += [numpy.array([[1,0,0],
                       [0,1,0],
                       [0,0,2]])]

# Simple shears
F_lst += [numpy.array([[1,1,0],
                       [0,1,0],
                       [0,0,1]])]
F_lst += [numpy.array([[1,0,1],
                       [0,1,0],
                       [0,0,1]])]
F_lst += [numpy.array([[1,0,0],
                       [0,1,1],
                       [0,0,1]])]
F_lst += [numpy.array([[1,0,0],
                       [1,1,0],
                       [0,0,1]])]
F_lst += [numpy.array([[1,0,0],
                       [0,1,0],
                       [1,0,1]])]
F_lst += [numpy.array([[1,0,0],
                       [0,1,0],
                       [0,1,1]])]

# Rotation
alpha = math.pi/2
F_lst += [numpy.array([[+math.cos(alpha),-math.sin(alpha),0],
                       [+math.sin(alpha),+math.cos(alpha),0],
                       [               0,               0,1]])]

for F in F_lst:
    print("F = " + str(F))

    # Assuming an homogeneous deformation gradient, compute the displacement and displacement gradient: u = x(X)-X = F.X-X = (F-1).X = GU.X with GU = F-1
    GU = F - numpy.eye(3)
    print("GU = " + str(GU))

    for k_point in xrange(8):
        farray_disp.SetTuple(k_point, numpy.dot(GU, points.GetPoint(k_point)))
        #print farray_disp.GetTuple(k_point)

    # Compute the small (linearized) strain tensor: e = (GU)_sym = (GU+GU^T)/2
    e = (GU + numpy.transpose(GU))/2
    print("e = " + str(e))

    # Compute the Green-Lagrange strain tensor: E = (F.F^T-1)/2 = (GU+GU^T+GU^T.GU)/2 = e + GU^T.GU/2
    E = e + numpy.dot(numpy.transpose(GU), GU)/2
    print("E = " + str(E))

    # Compute displacement gradient using vtkCellDerivatives
    cell_derivatives = vtk.vtkCellDerivatives()
    cell_derivatives.SetVectorModeToPassVectors()
    cell_derivatives.SetTensorModeToComputeGradient()
    cell_derivatives.SetInputData(ugrid_hex)
    cell_derivatives.Update()
    vector_gradient = numpy.reshape(cell_derivatives.GetOutput().GetCellData().GetArray("VectorGradient").GetTuple(0), (3,3))
    print("VectorGradient = " + str(vector_gradient))
    assert numpy.allclose(vector_gradient, GU)

    # Compute small strain tensor using vtkCellDerivatives
    cell_derivatives = vtk.vtkCellDerivatives()
    cell_derivatives.SetVectorModeToPassVectors()
    cell_derivatives.SetTensorModeToComputeStrain()
    cell_derivatives.SetInputData(ugrid_hex)
    cell_derivatives.Update()
    strain = numpy.reshape(cell_derivatives.GetOutput().GetCellData().GetArray("Strain").GetTuple(0), (3,3))
    print("Strain = " + str(strain))
    assert numpy.allclose(strain, e)

    # Compute Green-Lagrange strain tensor using vtkCellDerivatives
    cell_derivatives = vtk.vtkCellDerivatives()
    cell_derivatives.SetVectorModeToPassVectors()
    cell_derivatives.SetTensorModeToComputeGreenLagrangeStrain()
    cell_derivatives.SetInputData(ugrid_hex)
    cell_derivatives.Update()
    green_lagrange_strain = numpy.reshape(cell_derivatives.GetOutput().GetCellData().GetArray("GreenLagrangeStrain").GetTuple(0), (3,3))
    print("GreenLagrangeStrain = " + str(green_lagrange_strain))
    assert numpy.allclose(green_lagrange_strain, E)
