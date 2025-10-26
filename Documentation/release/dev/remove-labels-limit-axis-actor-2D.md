## vtkAxisActor2D no longer has a limit of the number of labels

Labels and actors are now dynamically allocated.
Therefore VTK_MAX_LABELS = 25 has been deprecated and is not used anymore.

LabelMappers, LabelActors, TitleTextProperty and LabelTextProperty have been moved from protected to private.
A public getter is now available for LabelMappers.
