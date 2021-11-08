## Add vtkDisplaySizedImplicitPlaneWidget

VTK now provides `vtkDisplaySizedImplicitPlaneWidget` which is an alternative to `vtkImplicitPlaneWidget2`.
While both provide similar functionality, `vtkDisplaySizedImplicitPlaneWidget` has the following key differences:

1) does not have an outline
2) the size of the normal arrow and plane are relative to the viewport size (that's why it's named display sized)
3) the origin can be moved freely by default, and it's not restricted by the bounding box
4) the size of the origin/cone handles are bigger
5) the plane is represented as a disk
6) tubing the perimeter is the only option
7) the disk radius can be resized by selecting and resizing the perimeter
8) the actors of the widget are highlighted only when they are touched, except from the disk plane surface
   1) disk plane surface is highlighted whenever any actor of the widget is touched
9) you can pick a new plane origin using o/O, by clicking at a point either on the plane or on an object rendered by the renderer
10) you can pick a new plane normal using n/N, by clicking at a point on an object rendered by the renderer
11) the picking tolerance of points is relative to the viewport size, which leads to better picking accuracy

`vtkDisplaySizedImplicitPlaneWidget` enables you to manipulate the plane and the objects that are rendered more
effectively, since you have full control of the size, position and direction of the plane.
