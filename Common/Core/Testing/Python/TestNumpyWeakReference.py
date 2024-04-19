import sys
import vtkmodules.test.Testing

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    vtkmodules.test.Testing.skip()

from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
import vtkmodules.numpy_interface.dataset_adapter as dsa

# Test getting an array associated with a dataset, creating a derived array in
# numpy and adding it back to the dataset. Verify that this doesn't create a
# cycle preventing the dataset from being collected.

source = vtkRTAnalyticSource()
source.Update()
dataset = dsa.DataSet(source.GetOutput())

data = dataset.PointData.GetArray(0)
# Create a derived array
newdata = numpy.int32(data)
# Add the derived array to the dataset
dataset.PointData.append(newdata, 'PleaseDontPinMe')
assert newdata.DataSet is not None
# Delete reference to the dataset
del dataset
del source
# The derived array reference to the dataset should now be None as it only held
# a weak reference to the original dataset
assert newdata.DataSet is None
