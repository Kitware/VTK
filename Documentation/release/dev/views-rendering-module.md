## New ViewsRendering module: views and representations

The new `VTK::ViewsRendering` module offers a higher level way to put data on
screen. Instead of assembling a render window, an interactor, an interactor
style, a geometry filter, a mapper and an actor for every dataset, you create a
view, hand it your data, and set the properties you care about.

```cpp
vtkNew<vtkStandardRenderView> view;
vtkNew<vtkSurfaceRepresentation> rep;
rep->SetInputConnection(source->GetOutputPort());
rep->SetColor(0.8, 0.2, 0.2);
rep->SetRepresentationToSurfaceWithEdges();
view->AddRepresentation(rep);
view->ResetCamera();
view->Start();
```

The same scene in Python, where `show()` creates the representation, connects
it, applies properties and adds it to the view in one call:

```python
from vtkmodules.vtkViewsRendering import vtkStandardRenderView

view = vtkStandardRenderView(window_title="Demo")
view.show(source, color="tomato", representation="surfacewithedges")
view.Start()
```

### vtkStandardRenderView

The view owns the renderer, render window and interactor, and starts with
defaults chosen so that a scene looks reasonable before you configure
anything: a trackball camera, a gradient background, orientation axes, and a
usable window size. Beyond that it provides:

* a light kit, with control over key light intensity, warmth, angle and the
  key to fill ratio
* background colors, gradient background, window size and title
* an interaction mode for camera manipulation and one for rubber band
  selection, and `SetInteractorStyle()` for an interactor style of your own,
  including a `vtkInteractorStyleManipulator` with your own button bindings
* surface selection through the hardware selector, or frustum selection, on
  points or on cells
* `SelectRegion()`, which selects a screen space region exactly as a rubber
  band drag would, so selection can be driven from application code or from a
  custom interactor style without synthesizing interactor events
* `SelectionChangedEvent` and `GetCurrentSelection()` for reacting to
  selections, and `ClearSelection()` for dropping them

### vtkSurfaceRepresentation

Surface geometry, with the extraction, mapping and actor properties gathered
behind one object. It handles composite datasets, AMR and hyper tree grids
through `vtkGeometryFilterDispatcher`.

* representation modes: points, wireframe, surface, surface with edges,
  outline and feature edges, one mutually exclusive choice rather than a mode
  plus a set of flags
* color, opacity, edge color and width, point size, lighting, and the physically
  based properties roughness and metallic
* coloring by a point, cell or field data array, optionally by one component,
  through a lookup table you supply or the default one
* a scalar bar
* selection display, with its own color, opacity, line width, point size and
  representation; without one of those the selection is drawn as points or as
  wireframe to match what was selected
* geometry options forwarded to the extraction filter: normal generation,
  feature angle, triangulation, nonlinear subdivision, process ids, and the
  AMR and composite specific options

The range that scalars are mapped through belongs to the lookup table rather
than to the representation, so a table shared between representations keeps one
range and a range you set on a table is never written over.

### vtkVolumeRepresentation

Volume rendering on `vtkSmartVolumeMapper`, with the same shape of API.

* color and opacity transfer functions generated from the scalar range of your
  data, so a volume is visible before you configure anything
* `SetColorTransferFunction()` and `SetScalarOpacity()` to take over, and
  `ResetColorTransferFunction()`, `ResetScalarOpacity()` and
  `ResetTransferFunctions()` to hand a function back and have it generated
  again from the data
* shading, ambient, diffuse, specular, interpolation type, blend mode and
  requested render mode
* a scalar bar

### Python

Representation and view classes carry the usual snake case properties, and
properties backed by `Set<Property>To<Name>()` methods accept the matching
string, so `rep.representation = "outline"` and
`view.interaction_mode = "selection"` work. On top of that:

```python
view = vtkStandardRenderView()
view.size = (1200, 800)

# show() builds a surface representation by default; ask for another kind by
# name or by class.
surface = view.show(sphere, color="steel_blue", opacity=0.8)
volume = view.show(wavelet, "volume", scalar_opacity_unit_distance=1.5)

view += existing_representation
view -= surface
```

Colors accept a snake case name from `vtkmodules.util.colors` as well as an
`(r, g, b)` tuple.

### Examples

`Examples/GUI/Imgui` contains examples built on these classes, covering
surfaces, volumes, partitioned data, selection and interactive property
editing. Each one carries a script header naming its dependencies, so that
once the module ships in a wheel they can be run directly with
`uv run <example>.py`.
