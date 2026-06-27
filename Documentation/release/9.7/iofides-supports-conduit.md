### `vtkFidesReader` supports reading from `Conduit` nodes

The `vtkFidesReader`, which historically only supported reading from ADIOS2 files, now allows you to pass in-memory Conduit nodes directly into the `vtkFidesReader`, bypassing the filesystem entirely.

**Expected Usage**
You can provide data sources via both C++ and Python. The reader relies on a JSON data model where the data source type is explicitly given as `"conduit"`.

In C++ (using the Conduit C-API):

```cpp
conduit_node* c_node = conduit::c_node(&my_cpp_node);
reader->SetDataSourceNode("source", c_node);
reader->Update();
```

In Python:

```python
# node is a standard conduit.Node populated with numpy arrays
reader.SetDataSourceNode("source", node)
reader.Update()
```

To clear a previously set resource, you can pass `nullptr` (or `None` in Python). Note that doing so will leave the reader in an invalid state until a new, valid data source is provided for the pipeline to update.

**Known Issues and Caveats**
* **Memory Ownership (C++):** The reader **does not** take ownership of the memory backing the `conduit_node`. You are strictly responsible for ensuring the data backing the node remains valid and unmodified until the VTK pipeline execution (`Update()`) has completely finished.
* **Memory Ownership (Python):** The reader will automatically increment the reference count of the provided Python `conduit.Node`, preventing standard garbage collection. However, if your Conduit node is just a view into externally managed memory (like a C++ simulation array or a volatile NumPy view), you must ensure that external memory is not freed while the reader holds the reference.
* **Partition Indexing:** Because Conduit nodes represent local in-memory snapshots, the resulting `vtkPartitionedDataSet` uses local index geometries (starting at `0`) rather than ADIOS2's globally offset indices.
* **Build Requirements:** While still present in the api, the Python overload of `SetDataSourceNode` is disabled if VTK is compiled without Python wrapping (`VTK_WRAP_PYTHON=OFF`) or if the underlying Conduit library was built without Python support. Similarly, the C++ overload of `SetDataSourceNode` is disabled if VTK is compiled without Conduit support.
