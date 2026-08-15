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
