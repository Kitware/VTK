## New ViewsScivis module: views and representations

The new `VTK::ViewsScivis` module assembles the renderer, render window,
interactor, lights and orientation axes that every rendering program otherwise
builds by hand. Create a view, add representations of your data, and set the
properties you care about; a new view draws something reasonable before any of
that.

```cpp
vtkNew<vtkSphereSource> source;
vtkNew<vtkScivisView> view;
vtkNew<vtkSurfaceRepresentation> rep;
rep->SetInputConnection(source->GetOutputPort());
rep->SetColor(1.0, 0.39, 0.28);
rep->SetRepresentationToSurfaceWithEdges();
view->AddRepresentation(rep);
view->Start();
```

```python
source = vtkSphereSource()
view = vtkScivisView(window_title="Demo")
view.show(source, color="tomato", representation="surfacewithedges")
view.Start()
```

- `vtkScivisView` holds the scene: background, window, interaction mode, light
  kit, and standard view directions (`ViewPositiveX()`, `ViewIsometric()`, ...,
  `SetViewDirection()`). Selection is `GetSelector()`, and screenshots and scene
  export (picked by file extension) are `GetExporter()`.
- `vtkSurfaceRepresentation` and `vtkVolumeRepresentation` draw data, with
  coloring by point, cell or field arrays, per-block visibility, color and
  opacity through `GetBlocks()`, and transfer functions generated from the data
  until you supply your own. `ColorBySolidColor()` turns array coloring off.
- `vtkTextOverlayRepresentation` draws text over the scene, and
  `vtkGridAxesRepresentation` draws axes that follow the scene's bounds.
- The view maintains a scalar bar for each array being drawn, through
  `GetScalarBars()`. Bars come and go with the scene, and their style
  (`SetLabelFontSize()`, `SetNumberOfLabels()`, `SetBarWidth()`, ...) is set on
  the set of bars so that it applies to bars created later.
- Colors come from a `vtkLookupTableManager` keyed by array name, so every
  representation of an array shares one map and range. Register a map there to
  choose it, and share a manager between views to keep them in step.

The grid axes and text overlay carry the properties applications set routinely,
such as font sizes, titles and colors. Those forward to the objects the classes
hand out (`GetTextProperty()`, `GetGridAxesActor()`), which remain the place for
anything finer, as `GetProperty()` and `GetVolumeProperty()` are for the line
width, point size and lighting of surfaces and volumes.

In Python, properties are snake case and accept names for enumerated values and
colors (`rep.representation = "outline"`, `rep.color = "steel_blue"`). A
property a class does not carry is routed to the object that owns it, and
constructors take dicts for sub-objects. `view += rep` and `view -= rep` add and
remove representations, and the collections behave like Python containers:

```python
for bar in view.scalar_bars: ...
view.scalar_bars["Temperature"].title = "T (K)"
view.lookup_table_manager["Temperature"] = my_map
rep.blocks[3].opacity = 0.5
```

Examples are in `Examples/Views/Scivis` (plain scripts) and `Examples/GUI/Imgui`
(interactive).
