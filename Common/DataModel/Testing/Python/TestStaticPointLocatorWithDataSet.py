#!/usr/bin/env python
import vtk

# This tests the static point locator on non-vtkPointSet
# datasets.
#

image = vtk.vtkImageData()
image.SetDimensions(50,50,50)

spl = vtk.vtkStaticPointLocator()
spl.SetDataSet(image)
spl.BuildLocator()
