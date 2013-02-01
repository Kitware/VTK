#!/usr/bin/env python

import sys
import math

for i in range(0, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = sys.path + [sys.argv[i+1]]

import vtk
from vtk.util.misc import vtkGetDataRoot

filename = vtkGetDataRoot() + '/Data/tetraMesh.vtk'

def DumpQualityStats(iq, arrayname):
    print "  cardinality:",
    print iq.GetOutput().GetFieldData().GetArray(arrayname).GetComponent(0, 4),
    print " ,  range:",
    print iq.GetOutput().GetFieldData().GetArray(arrayname).GetComponent(0, 0),
    print " - ",
    print iq.GetOutput().GetFieldData().GetArray(arrayname).GetComponent(0, 2)

    print "  average:",
    print iq.GetOutput().GetFieldData().GetArray(arrayname).GetComponent(0, 1),
    print " ,  standard deviation:",
    print math.sqrt(abs(iq.GetOutput().GetFieldData().GetArray(arrayname).GetComponent(0, 3)))

mr = vtk.vtkUnstructuredGridReader()
mr.SetFileName(filename)

iq = vtk.vtkMeshQuality()
iq.SetInputConnection(mr.GetOutputPort())
iq.VolumeOn()
iq.RatioOn()
iq.Update()

ug = iq.GetOutput()
iq.SetInputConnection(mr.GetOutputPort())

if ug.GetNumberOfCells():
    print
    print "Triangle quality of mesh"
    print mr.GetFileName()

    iq.SetTriangleQualityMeasureToEdgeRatio()
    iq.Update()
    print " Edge Ratio:"

    DumpQualityStats(iq, "Mesh Triangle Quality")

    iq.SetTriangleQualityMeasureToAspectRatio()
    iq.Update()
    print " Aspect Ratio:"

    DumpQualityStats(iq, "Mesh Triangle Quality")

    iq.SetTriangleQualityMeasureToRadiusRatio()
    iq.Update()
    print " Radius Ratio:"

    DumpQualityStats(iq, "Mesh Triangle Quality")

    iq.SetTriangleQualityMeasureToAspectFrobenius()
    iq.Update()
    print " Frobenius Norm:"

    DumpQualityStats(iq, "Mesh Triangle Quality")

    iq.SetTriangleQualityMeasureToMinAngle()
    iq.Update()
    print " Minimal Angle:"

    DumpQualityStats(iq, "Mesh Triangle Quality")

    print "Quadrilatedral quality of mesh"
    print mr.GetFileName()

    iq.SetQuadQualityMeasureToEdgeRatio()
    iq.Update()
    print " Edge Ratio:"

    DumpQualityStats(iq, "Mesh Quadrilateral Quality")

    iq.SetQuadQualityMeasureToAspectRatio()
    iq.Update()
    print " Aspect Ratio:"

    DumpQualityStats(iq, "Mesh Quadrilateral Quality")

    iq.SetQuadQualityMeasureToRadiusRatio()
    iq.Update()
    print " Radius Ratio:"

    DumpQualityStats(iq, "Mesh Quadrilateral Quality")

    iq.SetQuadQualityMeasureToMedAspectFrobenius()
    iq.Update()
    print " Average Frobenius Norm:"

    DumpQualityStats(iq, "Mesh Quadrilateral Quality")

    iq.SetQuadQualityMeasureToMaxAspectFrobenius()
    iq.Update()
    print " Maximal Frobenius Norm:"

    DumpQualityStats(iq, "Mesh Quadrilateral Quality")

    iq.SetQuadQualityMeasureToMinAngle()
    iq.Update()
    print " Minimal Angle:"

    DumpQualityStats(iq, "Mesh Quadrilateral Quality")

    print "Tetrahedral quality of mesh"
    print mr.GetFileName()

    iq.SetTetQualityMeasureToEdgeRatio()
    iq.Update()
    print " Edge Ratio:"

    DumpQualityStats(iq, "Mesh Tetrahedron Quality")

    iq.SetTetQualityMeasureToAspectRatio()
    iq.Update()
    print " Aspect Ratio:"

    DumpQualityStats(iq, "Mesh Tetrahedron Quality")

    iq.SetTetQualityMeasureToRadiusRatio()
    iq.Update()
    print " Radius Ratio:"

    DumpQualityStats(iq, "Mesh Tetrahedron Quality")

    iq.SetTetQualityMeasureToAspectFrobenius()
    iq.Update()
    print " Frobenius Norm:"

    DumpQualityStats(iq, "Mesh Tetrahedron Quality")

    iq.SetTetQualityMeasureToMinAngle()
    iq.Update()
    print " Minimal Dihedral Angle:"

    DumpQualityStats(iq, "Mesh Tetrahedron Quality")

    iq.SetTetQualityMeasureToCollapseRatio()
    iq.Update()
    print " Collapse Ratio:"

    DumpQualityStats(iq, "Mesh Tetrahedron Quality")

    print "Hexahedral quality of mesh"
    print mr.GetFileName()

    iq.SetHexQualityMeasureToEdgeRatio()
    iq.Update()
    print " Edge Ratio:"

    DumpQualityStats(iq, "Mesh Hexahedron Quality")
