## Dictionary interface for vtkDataArraySelection

`vtkDataArraySelection` now supports a Python dictionary interface. You can
get, set, and delete array enable/disable states using standard dictionary
syntax:

```python
sel = reader.GetPointDataArraySelection()
sel["Pressure"] = True
if "Velocity" in sel:
    enabled = sel["Velocity"]
for name, state in sel.items():
    print(name, state)
```
