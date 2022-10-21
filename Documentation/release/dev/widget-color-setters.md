## More uniform color setters for widgets

ParaView needs VTK widgets to have the colors used in widgets changeable via a
standard setter, as opposed to using `GetProperty()`, so that the colors can
be linked to the ParaView palette using proxy xml.

Add more uniform color setters to several widgets used by ParaView:
`SetForegroundColor()`, `SetHandleColor`, and
`SetInteractionColor`. A few widgets have default colors changed to more
closely match most widgets.

The intended use of these colors is as follows:
| Color       | Description |
| ----------- | ----------- |
| `HandleColor`      | Widget handles that are available to interact with via click+drag.                   |
| `InteractionColor` | Widget handles the user is interacting with (via a click+drag) or hovering over.     |
| `ForegroundColor`  | Widget elements meant to contrast with the background and which are not interactive. |

When hovering, the `InteractionColor` can also be used to show which parts
of the widget will change if this handle is dragged. For instance, using the
`vtkDisplaySizedImplicitPlaneRepresentation`, hovering the axis also displays
the plane disc in the `InteractionColor`, to show it will change when the
axis is rotated.
