#!/usr/bin/env python

import sys

for i in range(0, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = sys.path + [sys.argv[i+1]]

from vtkpython import *

filename = vtkGetDataRoot() + '/Data/tetraMesh.vtk'

reader = vtkUnstructuredGridReader()
reader.SetFileName(filename)

a = vtkMeshQuality()
a.SetInput(reader.GetOutput())
a.VolumeOn()
a.RatioOn()
a.Update()


mesh = a.GetOutput().GetFieldData().GetScalars()

for i in range(mesh.GetNumberOfTuples()):
	print mesh.GetTuple2(i)
	
sys.exit(0)
