## vtkPolarAxesActor improvements

You can now use `vtkAxisActor::UseText3D` mode from the `vtkPolarAxesActor`. In this mode,
the text are a rasterized image in the 3D space.

Several fixes were needed:
 * `vtkPolarAxesActor` should accept translucent pass
 * `vtkPolarAxesActor` should forward PropertyKeys to other actors (to correctly configure render passes)
 * when using `vtkVectorText`, the actor used has no text property. Mimic font size with scaling.
