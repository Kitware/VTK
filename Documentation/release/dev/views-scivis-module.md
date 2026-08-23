## New ViewsScivis module: views and representations

Every VTK rendering program builds the same thing by hand: a renderer, a render
window, an interactor, an interactor style, a light, an orientation marker and
the wiring between them, then a geometry filter, a mapper and an actor for every
dataset. The new `VTK::ViewsScivis` module is that assembly, already built. You
create a view, hand it your data, and set the properties you care about. A view
you have just created draws something reasonable before you configure anything,
and it stays consistent when you replace a piece of it.

```cpp
vtkNew<vtkScivisView> view;
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
from vtkmodules.vtkViewsScivis import vtkScivisView

view = vtkScivisView(window_title="Demo")
view.show(source, color="tomato", representation="surfacewithedges")
view.Start()
```

### How the classes fit together

A view shows representations, and both are families of their own -- they are not
`vtkView` and `vtkDataRepresentation`, and do not mix with the information
visualization classes that are.

A representation is a choice of how to show something: the same dataset shown as
a surface and shown as a volume is two representations. Everything in a scene is
one, so everything is created, configured, added, shown and hidden in the same
way, and a view holds them all in a single list.

That is worth keeping cheap, so `vtkScivisRepresentation` asks almost nothing: a
representation can be shown or hidden, and can put itself into a view and take
itself out again. An annotation -- a text overlay, a scale bar -- is showable in
exactly that sense, and gains nothing from having to carry an input dataset, a
color map and a selection round trip in order to say so.

Those belong to `vtkScivisDataRepresentation`, which is what a representation of
actual data derives from. It requires the answers a view needs in order to act on
a scene rather than on one object: the bounds to frame the camera on, the array
being drawn and its range, and the color map to draw it through. Because every
data representation has to answer, the view can frame the camera on all of them
at once, and one color map can span several.

What lives where follows one line: whatever is true of the scene as a whole is on
the view, whatever is true of one thing being drawn is on its representation.
Neither mirrors the objects underneath it -- where you need more than they offer,
both hand out the object that owns what you are after.

Selection is `vtkScivisSelector`, reached as `view->GetSelector()`: how a
screen region becomes a selection, whether points or cells come back, and what
was picked last. The view keeps only the interaction half, which interactor
style is installed.

### vtkScivisView

The defaults are a trackball camera, a gradient background, orientation axes and
a usable window size. The view keeps the assembly consistent as you replace parts
of it: hand it a render window of your own and the renderers move across, while
an interactor style you had installed survives the change. Beyond that it
provides:

* background colors, gradient background, window size and title
* an interaction mode for camera manipulation and one for rubber band
  selection, and `SetInteractorStyle()` for an interactor style of your own,
  including a `vtkInteractorStyleManipulator` with your own button bindings
* a light kit, switched on and off here and configured through `GetLightKit()`;
  lights of your own go on the renderer and are left alone either way
* selection, in an object of its own -- see below

`GetRenderer()` takes lights and props of your own, and
`GetOrientationMarkerWidget()` the axes marker's viewport and marker.

### vtkSurfaceRepresentation

Surface geometry, with the extraction, mapping and actor properties gathered
behind one object. It handles composite datasets, AMR and hyper tree grids
through `vtkGeometryFilterDispatcher`.

* representation modes: points, wireframe, surface, surface with edges,
  outline and feature edges, one mutually exclusive choice rather than a mode
  plus a set of flags
* color, opacity and edge color
* coloring by a point, cell or field data array, optionally by one component,
  through a color map you supply or the default one, and whether an array of
  colors is mapped or drawn as it is
* per-block visibility, color and opacity for composite input, through
  `GetBlocks()`
* how a selection made in the view is drawn over the surface

Everything else belongs to the objects it hands out: line width, point size,
lighting and the physically based properties on `GetProperty()`; normal
generation, feature angle, triangulation, subdivision and the AMR options on
`GetGeometryFilter()`.

The range that scalars are mapped through belongs to the color map rather than
to the representation, so a map shared between representations keeps one range
and a range you set on a map is never written over.

### vtkVolumeRepresentation

Volume rendering on `vtkSmartVolumeMapper`, with the same shape of API.

* color and opacity transfer functions generated from the scalar range of your
  data, so a volume is visible before you configure anything
* `SetColorTransferFunction()` and `SetScalarOpacity()` to take over, and
  `ResetColorTransferFunction()`, `ResetScalarOpacity()` and
  `ResetTransferFunctions()` to hand a function back and have it generated
  again from the data
* whether the volume is lit, and its scalar opacity unit distance
* which array is rendered

Ambient, diffuse, specular and interpolation type live on `GetVolumeProperty()`,
blend mode and requested render mode on `GetVolumeMapper()`.

### Python

Representation and view classes carry the usual snake case properties, and
properties backed by `Set<Property>To<Name>()` methods accept the matching
string, so `rep.representation = "outline"` and
`view.interaction_mode = "selection"` work. On top of that:

```python
view = vtkScivisView()
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

A property the C++ class does not carry itself is routed to the object that
owns it, so the flat spelling that makes keyword arguments read well survives
the smaller C++ API:

```python
rep.specular = 0.3          # the actor's property
view.key_light_intensity = 0.8   # the light kit
```

Constructors take the same, and a dict configures a sub-object -- there is no
way to spell a nested path as a keyword name:

```python
view = vtkScivisView(
    window_title="Demo",
    selector={"mode": "frustum"},
    light_kit={"key_light_intensity": 0.8},
)
```

Both forms check what they are given, so a misspelled property raises rather
than quietly becoming an attribute that does nothing.

### Examples

`Examples/GUI/Imgui` contains examples built on these classes, covering
surfaces, volumes, partitioned data, selection and interactive property
editing. Each one carries a script header naming its dependencies, so that
once the module ships in a wheel they can be run directly with
`uv run <example>.py`.
