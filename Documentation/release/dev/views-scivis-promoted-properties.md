## ViewsScivis: the properties you actually set are on the class

Representations and components hand out the objects they draw with rather than
mirroring them, which keeps their API small. Taken to the letter it made the
common case awkward: setting the size of the numbers along a set of axes read

```cpp
axes->GetGridAxesActor()->GetLabelTextProperty(0)->SetFontSize(24);   // and again for 1 and 2
```

The properties applications reach for routinely are now on the class:

```cpp
axes->SetLabelFontSize(24);
axes->SetXTitle("x (m)");
text->SetFontSize(36);
surface->SetLineWidth(3.0);
volume->SetAmbient(0.3);
view->GetScalarBars()->SetLabelFontSize(22);
```

| Class | Promoted |
| --- | --- |
| `vtkGridAxesRepresentation` | `XTitle`, `YTitle`, `ZTitle`, `LabelFontSize`, `TitleFontSize`, `LabelColor`, `TitleColor` |
| `vtkScivisScalarBars` | `TitleFontSize`, `LabelFontSize`, `TextColor`, `NumberOfLabels`, `BarWidth`, `BarHeight` |
| `vtkTextOverlayRepresentation` | `FontSize`, `Color`, `Bold`, `Italic` |
| `vtkSurfaceRepresentation` | `LineWidth`, `PointSize`, `Specular`, `SpecularPower` |
| `vtkVolumeRepresentation` | `Ambient`, `Diffuse`, `Specular` |

Each is a forwarder, not a copy: setting one reaches the same object
`GetGridAxesActor()`, `GetTextProperty()`, `GetProperty()` or
`GetVolumeProperty()` hands out, and reading one reports what that object says.
Changing it either way gives the same answer both ways.

The font sizes and colors on the axes apply to all three at once, which is what
an application that cares about the size of its axis text wants. A different
font on one axis, a label format, a shadow — anything finer stays on the object
underneath, or the small API would have been for nothing.

### Scalar bars are the case this was really for

A view makes scalar bars as arrays start being drawn, so an application cannot
reach a bar to configure it before it exists, and anything set on one is lost
when that bar is retired. The style is therefore a property of the *set*, and a
bar made later is drawn the same way:

```cpp
view->GetScalarBars()->SetLabelFontSize(22);   // before anything is drawn
view->GetScalarBars()->SetNumberOfLabels(9);
```

`BarWidth` and `BarHeight` say how much of the viewport a bar takes up. Bars are
stacked down the right hand edge at that size, shrinking below `BarHeight` when
there are more of them than there is room for.

### Collections read like collections

Anything the module holds a set of is a Python container. The bars a view is
maintaining, and the representations it is showing, are sequences:

```python
print(f"{len(view.scalar_bars)} bar(s)")
for bar in view.scalar_bars:
    print(bar.title, bar.lookup_table.range)

for representation in view:
    ...
```

which goes with the `view += representation` the view already took. A bar can be
reached by the array it is labelled with rather than by position, with the field
association where an array is drawn from both:

```python
view.scalar_bars["Temperature"].title = "T (K)"
view.scalar_bars["Temperature", vtkDataObject.FIELD_ASSOCIATION_CELLS]
"Temperature" in view.scalar_bars
```

The lookup table manager is the mapping of array name to color map that it
already was:

```python
view.lookup_table_manager["Temperature"] = my_map
"Temperature" in view.lookup_table_manager
for name in view.lookup_table_manager:
    ...
del view.lookup_table_manager["Temperature"]
```

Reading a name that has no map makes one, the way `collections.defaultdict`
does, because that is what the manager is for. `in` asks without making one.

Per-block properties are indexed by block, so the flat index is written once
rather than repeated in every call:

```python
rep.blocks[3].visibility = False
rep.blocks[3].color = "tomato"
rep.blocks[3].opacity = 0.5
```

### Turning array coloring off has a name

`ColorByPointArray()`, `ColorByCellArray()` and `ColorByFieldArray()` had no
counterpart, so going back to a solid color meant knowing that scalar visibility
was the switch behind them. `vtkSurfaceRepresentation::ColorBySolidColor()`
completes the family and leaves the color alone, so the one set while an array
was being drawn is what comes back.
