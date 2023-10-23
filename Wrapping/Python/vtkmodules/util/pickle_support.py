"""This module generates support for pickling vtkDataObjects from python.
It needs to be imported specifically in order to work:

>>> import vtkmodules.util.pickle_support

Once imported however, the pickling of data objects is very straighforward. Here is an
example using poly data:

>>> sphereSrc = vtkSphereSource()
>>> sphereSrc.Update()
>>> pickled = pickle.dumps(sphereSrc.GetOutput())
>>> unpickled = pickle.loads(pickled)
>>> print(unpickled)
*description of sphere data set*

The underlying serialization of the vtkDatObjects is based on the marshaling capabilities
found in vtkCommunicator. Importing this module adds entries for the most common data
objects in the global dispatch table used by pickle. NumPy is required as well since the
-serialized data object gets pickled as a numpy array.
"""

try:
    import copyreg, pickle, numpy
except ImportError:
    raise RuntimeError("This module depends on the pickle, copyreg, and numpy modules.\
 Please make sure that it is installed properly.")

from ..vtkParallelCore import vtkCommunicator
from ..vtkCommonCore import vtkCharArray
from .. import vtkCommonDataModel

def unserialize_VTK_data_object(state):
    """Takes a state dictionary with entries:
      - Type : a string with the class name for the data object
      - Serialized : a numpy array with the serialized data object

      and transforms it into a data object.
    """

    if ("Type" not in state.keys()) or ("Serialized" not in state.keys()):
        raise RuntimeError("State dictionary passed to unpickle does not have Type and/or\
 Serialized keys.")

    new_data_object = None
    DataSetClass = None
    try:
        DataSetClass = getattr(vtkCommonDataModel, state["Type"])
    except:
        raise TypeError("Could not find type " + type_string + " in vtkCommonDataModel module")
    serialized_data = state["Serialized"]
    new_data_object = DataSetClass()
    char_array = vtkCharArray()
    char_array.SetVoidArray(serialized_data, memoryview(serialized_data).nbytes, 1)
    if vtkCommunicator.UnMarshalDataObject(char_array, new_data_object) == 0:
        raise RuntimeError("Marshaling data object failed")
    return new_data_object

def serialize_VTK_data_object(data_object):
    """Returns a tuple with a reference to the unpickling function and a state dictionary
    with entires:
      - Type : a string with the class name for the data object
      - Serialized : a numpy array with the serialized data object

      This is exactly the state dictionary that unserialize_VTK_data_object expects.
    """

    if not data_object.IsA("vtkDataObject"):
        raise TypeError("Object passed to pickling should be a vtkDataObject")
    data_object_type = data_object.GetClassName()
    char_array = vtkCharArray()
    if vtkCommunicator.MarshalDataObject(data_object, char_array) == 0:
        raise RuntimeError("UnMarshaling data object failed")
    return unserialize_VTK_data_object, (
        { "Type" : data_object_type,
          "Serialized" : numpy.frombuffer(char_array, numpy.int8, char_array.GetNumberOfValues()) },)


# Fill in global dispatch table for most vtkDataObject types
copyreg.pickle(vtkCommonDataModel.vtkDataSet, serialize_VTK_data_object)

copyreg.pickle(vtkCommonDataModel.vtkPolyData, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkUnstructuredGrid, serialize_VTK_data_object)

copyreg.pickle(vtkCommonDataModel.vtkImageData, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkRectilinearGrid, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkStructuredGrid, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkExplicitStructuredGrid, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkStructuredPoints, serialize_VTK_data_object)

copyreg.pickle(vtkCommonDataModel.vtkUniformGridAMR, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkOverlappingAMR, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkHierarchicalBoxDataSet, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkNonOverlappingAMR, serialize_VTK_data_object)

copyreg.pickle(vtkCommonDataModel.vtkTable, serialize_VTK_data_object)

copyreg.pickle(vtkCommonDataModel.vtkTree, serialize_VTK_data_object)

copyreg.pickle(vtkCommonDataModel.vtkCompositeDataSet, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkDataObjectTree, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkMultiBlockDataSet, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkPartitionedDataSet, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkPartitionedDataSetCollection, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkMultiPieceDataSet, serialize_VTK_data_object)

copyreg.pickle(vtkCommonDataModel.vtkDirectedGraph, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkUndirectedGraph, serialize_VTK_data_object)
copyreg.pickle(vtkCommonDataModel.vtkMolecule, serialize_VTK_data_object)
