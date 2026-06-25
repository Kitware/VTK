## Look up algorithms in a Pipeline by `object_name`

`Pipeline` (the result of any `>>` chain in `vtkmodules.util.execution_model`) now supports `pipeline[name]` lookup. The pipeline graph is traversed from the last algorithm backward through every input port and every connection per port, and the first algorithm whose
`GetObjectName()` matches *name* is returned. A missing name raises
`KeyError`.

This lets long or branching pipelines be inspected and reconfigured
by name instead of by holding on to every intermediate algorithm
reference:

```python
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkFiltersGeneral import vtkShrinkFilter
from vtkmodules.vtkFiltersCore import vtkElevationFilter

p = (
    vtkSphereSource(object_name="src", radius=2)
    >> vtkShrinkFilter(object_name="shrink", shrink_factor=0.7)
    >> vtkElevationFilter(object_name="elev")
)

# Tweak parameters by name later in the script
p["src"].SetThetaResolution(64)
p["shrink"].SetShrinkFactor(0.9)

p()  # re-execute
```

The traversal follows *all* input ports and *all* connections, so
appending and merging branches both work. Any algorithm reachable from
the pipeline tail is findable; algorithms outside the pipeline (e.g.
disconnected sources) are not.

Cycles are guarded against with an identity-based visited set, so a
malformed graph cannot cause an infinite loop.

`object_name` is set as a constructor keyword (`vtkSphereSource(object_name="src")`) or via `SetObjectName()` on any
`vtkObject`; no new infrastructure is required.

The classic API (`Pipeline.last`, `Pipeline.first`, manual traversal
via `GetInputConnection`) is unchanged — the new `__getitem__` is
purely additive.
