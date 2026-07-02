## Pythonic API for VTK widgets

The 3D interaction widgets in `vtkInteractionWidgets` now have a Pythonic
API that removes most of the boilerplate normally required to wire up a
widget. Ten widget classes participate:

- `vtkBoxWidget2`
- `vtkSphereWidget2`
- `vtkImplicitPlaneWidget2`
- `vtkImplicitCylinderWidget`
- `vtkImplicitConeWidget`
- `vtkImplicitAnnulusWidget`
- `vtkLineWidget2`
- `vtkSplineWidget2`
- `vtkCoordinateFrameWidget`
- `vtkSliderWidget`

The new behavior is enabled automatically when `vtkInteractionWidgets` is
loaded — no extra import is needed.

### Constructor keyword arguments

Widgets accept their initial geometry, the interactor, and (where
applicable) per-widget parameters as keyword arguments. The default
representation is created and configured for you:

```python
from vtkmodules.vtkInteractionWidgets import (
    vtkImplicitPlaneWidget2, vtkSphereWidget2, vtkSliderWidget,
)

plane = vtkImplicitPlaneWidget2(
    interactor=iren,
    bounds=actor.GetBounds(),
    origin=(0, 0, 0),
    normal=(0, 0, 1),
)

sphere = vtkSphereWidget2(
    interactor=iren,
    bounds=(-1, 1, -1, 1, -1, 1),
    center=(0, 0, 0),
    radius=0.5,
)

slider = vtkSliderWidget(
    interactor=iren,
    min_value=0.0, max_value=1.0, value=0.5,
    title="Opacity",
)
```

For comparison, the equivalent classic-style setup for the plane widget
required four explicit calls (`CreateDefaultRepresentation`,
`SetInteractor`, `PlaceWidget`, `SetNormal`).

### Attribute forwarding to the representation

Widget instances forward unknown attribute access to their representation
via `__getattr__`, so the common pattern of pulling state off the
representation no longer requires `widget.GetRepresentation().GetX()`:

```python
plane = vtkImplicitPlaneWidget2(bounds=actor.GetBounds(), normal=(0, 0, 1))

# Equivalent — but no GetRepresentation() needed:
n = plane.GetNormal()
o = plane.GetOrigin()
```

The widget's own methods (e.g. `GetEnabled`, `SetInteractor`) take
priority over the representation's, so behavior is unchanged for methods
that exist on both.

### Decorator-style event callbacks

`on_interaction` and `on_end_interaction` register observers for the
corresponding VTK events and can be used as decorators:

```python
from vtkmodules.vtkCommonDataModel import vtkPlane

plane_func = vtkPlane()
widget = vtkImplicitPlaneWidget2(interactor=iren, bounds=actor.GetBounds(),
                                 normal=(0, 0, 1))

@widget.on_interaction
def update():
    widget.GetPlane(plane_func)
    cutter.SetCutFunction(plane_func)
    render_window.Render()

@widget.on_end_interaction
def commit():
    print("final normal:", widget.GetNormal())
```

The decorated function is returned unchanged, so it can still be called
directly or reused as a regular callback.

### Context manager and chaining

Widgets implement the context manager protocol — entering enables the
widget, exiting disables it — and `on()` / `off()` return `self` for
chaining:

```python
with vtkBoxWidget2(interactor=iren, bounds=actor.GetBounds()) as box:
    iren.Start()    # box is enabled inside the block, disabled on exit

# or, equivalently:
box = vtkBoxWidget2(interactor=iren, bounds=actor.GetBounds()).on()
...
box.off()
```

### Informative `repr`

Each widget produces a `repr` that includes the most relevant geometric
state, which is helpful at the REPL:

```python
>>> vtkSphereWidget2(bounds=(-1, 1, -1, 1, -1, 1), center=(1, 2, 3), radius=5)
vtkSphereWidget2(center=(1.0, 2.0, 3.0), radius=5.0)

>>> vtkImplicitConeWidget(bounds=(-1, 1, -1, 1, -1, 1),
...                       origin=(0, 0, 0), axis=(1, 0, 0), angle=30)
vtkImplicitConeWidget(origin=(0.0, 0.0, 0.0), axis=(1.0, 0.0, 0.0), angle=30.0)
```

### Backwards compatibility

The classic API (`CreateDefaultRepresentation`, `SetInteractor`,
`AddObserver`, `On` / `Off`) continues to work exactly as before;
existing scripts are unaffected. The Pythonic features are purely
additive and are installed via the standard `@vtkClassName.override`
mechanism.
