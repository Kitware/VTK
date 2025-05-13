# Python VTK data model

With the improvement done on the VTK Python wrapping API that are covered in the blogs [VTK and NumPy – a new take](https://www.kitware.com/vtk-and-numpy-a-new-take/) and [VTK 9.4: A Step Closer to the Ways of Python](https://www.kitware.com/vtk-9-4-a-step-closer-to-the-ways-of-python/), we enabled the use VTK in a more efficient way via Python.

But some of those new capabilites were incomplete or contained bugs. With VTK 9.5 we took another step toward cleaning field array handling so it can work homogenously between vtkDataSet and composite datasets.

```python
dataset = ...

# Check number of arrays available
if len(dataset.point_data):
    print("We have some fields on our points")

    # Check if a given name is available
    if "pressure" in dataset.point_data:
        np_array = dataset.point_data["pressure"]
        ...

    # Iterate over all array names (like a dict)
    for field_name in dataset.point_data:
        print(f" - {field_name} [on points]")

    # Iterate over all array names (keys)
    for field_name in dataset.point_data.keys():
        print(f" - {field_name} [on points]")

    # Iterate over all array values (values)
    for np_array in dataset.point_data.values():
        ...

    # Iterate over all arrays (key, value)
    for field_name, np_array in dataset.point_data.items():
        ...

```
