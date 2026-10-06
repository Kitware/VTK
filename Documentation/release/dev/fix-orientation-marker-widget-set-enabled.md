## Enabling the orientation marker widget without an interactor no longer crashes

`vtkOrientationMarkerWidget::SetEnabled()` used to log an error and carry on
when no interactor was set. It now reports the error and returns.

No-op calls to this method return early instead of checking whether the interactor is non-null.
