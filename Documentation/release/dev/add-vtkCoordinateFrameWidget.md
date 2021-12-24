## Add vtkCoordinateFrameWidget

VTK now has the `vtkCoordinateFrameWidget`which controls 3 orthogonal, right-handed planes.

![Coordinate Frame Widget](../imgs/dev/coordinateFrameWidget.png "Coordinate Frame Widget")

Axes are rendered proportional to viewport size, using a scale factor you can adjust.
Interaction methods allow you to pick the base point, align any axis with a surface normal,
and align any axis along the line from the base point to a picked point.
See [this discourse topic](https://discourse.vtk.org/t/vtkcoordinateframewidget/) for more information.
