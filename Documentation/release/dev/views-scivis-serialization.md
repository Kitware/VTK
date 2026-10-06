## ViewsScivis views and representations are serializable

The `VTK::ViewsScivis` module now opts into automatic marshalling, so the
state of a `vtkScivisView` and the representations it shows can be serialized
and restored, and the full public API of each class is reachable through the
generated method invokers. A remote session can build a Scivis scene and drive
it: add and remove representations, recolor them, move the camera, through
the same API used by the in-process code.

The classes covered:

* `vtkScivisView` -- background, window size and title, light kit,
  orientation axes, interaction mode, and the shared lookup table manager
* `vtkScivisRepresentation`, `vtkScivisDataRepresentation`, and their concrete
  forms, `vtkSurfaceRepresentation`, `vtkVolumeRepresentation`, and
  `vtkTextOverlayRepresentation`
* `vtkScivisSelector`, `vtkScivisScalarBars`, `vtkScivisExporter`,
  `vtkLookupTableManager`, and `vtkBlockProperties`

Two notes on what serializes and deserializes with a view:

* The scalar bars and the exporter are reached through the view but have no
  setter, so they are excluded from the view's state. Serialize them directly
  when their settings need to travel with a scene; their own methods remain
  callable through their invokers either way.
* `vtkLookupTableManager` serializes its color scheme and table size. The
  tables themselves are rebuilt from that scheme when the scene is next
  drawn, and ranges are recomputed from the data being drawn.

`vtkScivisView` also now has array-taking overloads, `GetBackground(double[3])`,
`GetBackground2(double[3])`, and `GetWindowSize(int[2])`, and
`vtkSurfaceRepresentation` gains `GetColor(double[3])` and
`GetEdgeColor(double[3])`, so a property read no longer needs a scratch
pointer.
