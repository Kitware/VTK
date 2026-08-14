## ViewsScivis: standard view directions

`vtkScivisView` can look at the scene from a standard direction and frame it in
one call, following the conventions ParaView uses:

```cpp
view->ViewPositiveX();   // the right side, up +Z
view->ViewNegativeY();   // the back, up +Z
view->ViewPositiveZ();   // the top, up +Y
view->ViewIsometric();   // 45 degrees round, about 35 degrees up
```

`SetViewDirection(look, up)` is the general form the six axis-aligned presets are
built from — the direction runs from the camera towards what it is looking at.
Each of these leaves the camera framed on everything the representations are
drawing, exactly as `ResetCamera()` does.

`GetCamera()` hands out the camera for anything finer: a parallel projection, a
view angle, or a position the application works out for itself. It is the same
camera the scene is drawn with, so
`view->GetRenderer()->GetActiveCamera()` and `view->GetCamera()` are one object.

Note that a camera move shows up in the view's modified time, because the camera
belongs to the renderer and the renderer's modified time is part of the view's.
That is true of any camera move, including one made through `GetCamera()` and one
the user makes by dragging — the standard directions are not special in this.
