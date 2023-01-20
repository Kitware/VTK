## Python wrapped `vtkDataObject`s now support pickling by the python `pickle` module

The Python `pickle` module (https://docs.python.org/3/library/pickle.html) is often used to serialize python objects to disk for later loading. You can now use the python `pickle` module to both pickle and unpickle `vtkDataObject`s python wrapped objects. The serialization of the data object uses the marshaling capabilities of `vtkCommunicator` behind the scenes.

In order to use this new feature in python, you must first run:
```python
import vtkmodules.util.pickle_support
```

Once you have imported the module the pickling of data objects is completely transparent. Here is a code snippet:

```python
from vtkmodules.vtkFiltersSources import vtkSphereSource
import vtkmodules.util.pickle_support
import pickle

sphereSrc = vtkSphereSource()
sphereSrc.Update()

pickled = pickle.dumps(sphereSrc.GetOutput())

unpickled = pickle.loads(pickled)

print(unpickled)
```
