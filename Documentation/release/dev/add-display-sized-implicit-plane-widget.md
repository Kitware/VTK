## Add vtkDisplaySizedImplicitPlaneWidget

VTK now provides `vtkDisplaySizedImplicitPlaneWidget` which is an alternative to `vtkImplicitPlaneWidget2`.
While both provide similar functionality, `vtkDisplaySizedImplicitPlaneWidget` has the following key differences:

1) the outline is not drawn by default
   1) If the outline is drawn, there an option to draw the intersection edges of the outline with the plane.
2) the size of the normal arrow and plane are relative to the viewport size (that's why it's named display sized)
    1) there is an option which allows the maximum size of the plane radius and normal arrow to be constrained by the widget bounds
3) the origin can be moved freely by default, and it's not restricted by the bounding box
4) the size of the origin/cone handles are bigger
5) the plane is represented as a disk
6) tubing the perimeter is the only option
7) the disk radius can be resized by selecting and resizing the perimeter
8) the actors of the widget are highlighted only when they are touched, except from the disk plane surface
   1) disk plane surface is highlighted whenever any actor of the widget is touched
9) you can pick a new plane origin using P/p, by picking a point intersected by a cell from an object rendered by the renderer
   1) If no such point is found, the camera plane focal point can optionally be returned.
10) you can pick a new plane origin using ctrl + P/p, by snapping to the closest point from an object rendered by the renderer
    1) If no such point is found, the camera plane focal point can optionally be returned.
11) you can pick a new plane normal using N/n, by picking a point's normal intersected by a cell from an object rendered by the renderer
    1) If no such normal is found, the camera plane normal can optionally be returned.
12) you can pick a new plane normal using ctrl + N/n, by snapping to the closest point's normal from an object rendered by the renderer
    2) If no such normal is found, the camera plane normal can optionally be returned.
13) the picking tolerance of points is relative to the viewport size, which leads to better picking accuracy

`vtkDisplaySizedImplicitPlaneWidget` enables you to manipulate the plane and the objects that are rendered more
effectively, since you have full control of the size, position and direction of the plane.
