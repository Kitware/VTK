#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkStaticPointLocator,
)

# This tests the static point locator on non-vtkPointSet
# datasets.
#

image = vtkImageData()
image.SetDimensions(50,50,50)

spl = vtkStaticPointLocator()
spl.SetDataSet(image)
spl.BuildLocator()
