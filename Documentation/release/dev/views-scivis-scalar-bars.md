## ViewsScivis: scalar bars belong to the view

A `vtkScivisView` now maintains a scalar bar for each array its representations
are drawing. Nothing has to be switched on: color by an array and a bar labelled
with it appears.

```cpp
vtkNew<vtkScivisView> view;
vtkNew<vtkSurfaceRepresentation> rep;
rep->SetInputConnection(source->GetOutputPort());
rep->ColorByPointArray("Temperature");
view->AddRepresentation(rep);
view->Start();                       // a "Temperature" bar is already there
```

A scalar bar describes the scene rather than any one representation, which is
why the view owns it. Two representations coloring by "Temperature" are one bar
spanning both of their ranges, not two bars disagreeing about the same quantity.
The bar goes away when the last representation drawing that array is removed or
hidden, and a live bar's range only ever grows, so a bar does not jump about as
data moves in and out of the scene.

Arrays in field data get no bar: field data carries one tuple per block rather
than one per element, so a bar over it would be labelling a range with no extent
in the scene.

The bars are a component of their own, `view->GetScalarBars()`. `GetActor()`, by
index or by array name and field association, reaches a bar to adjust its title,
label format or size. The set of bars is not
yours to change — the view adds and removes them as representations come and go.
`AutoVisibilityOff()` removes the bars and stops them being maintained, leaving
the representations and their color maps alone. `DraggableOn()` lets the user pick
a bar up and move it. A bar that has been moved stays where it was put rather than
being stacked with the rest, and stays there when dragging is turned off again —
turning it off takes away the handles, not what was done with them.

### vtkLookupTableManager

The view colors through a `vtkLookupTableManager`, a registry of lookup tables
keyed by array name. Every representation drawing a given array is offered the
same table, which is what makes two representations of "Temperature" come out the
same color and span one range.

The offer is not an instruction. A representation that owns transfer functions of
its own — a volume — declines it and goes on using its own, and the bar then
shows those instead. An array drawn only by such a representation leaves no table
behind in the manager.

The manager is where an application chooses the map an array is drawn through.
Setting one on a representation does not stick — the view offers the array's
shared map again on every render — so register it instead, and every
representation of that array is handed it:

```cpp
view->GetLookupTableManager()->SetLookupTable("Temperature", myMap);
```

A table registered with the manager keeps the range it was given; one the view
creates is spanned over the data. Share a manager between views to keep their
coloring in step:

```cpp
vtkNew<vtkLookupTableManager> manager;
first->SetLookupTableManager(manager);
second->SetLookupTableManager(manager);
```

A view that is not given one has its own, so views are independent by default.
