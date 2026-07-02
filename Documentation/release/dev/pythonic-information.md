## Pythonic API for vtkInformation and vtkInformationVector

`vtkInformation` and `vtkInformationVector` now expose a dictionary/list-style
Python API, so information objects can be manipulated with familiar Python
idioms instead of the verbose `Get`/`Set`/`Has`/`Remove` calls.

`vtkInformation` behaves like a mapping keyed by information key (a
`vtkInformationKey` object, or its name as a case-insensitive string):

```python
info["GUI_HIDE"] = True
spacing = info[vtkDataObject.SPACING()]
if "SPACING" in info:
    del info["SPACING"]
for key in info:
    ...
```

`vtkInformationVector` behaves like a list of information objects, supporting
indexing, iteration, `len()`, and `append()`.

The string-name lookup is backed by a thread-safe `vtkInformationKeyLookup`
registry populated as information keys are constructed.
