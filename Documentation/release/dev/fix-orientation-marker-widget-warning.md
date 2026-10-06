## Clearer warning when toggling orientation marker interactivity

Changing the interactive state of `vtkOrientationMarkerWidget` before an
interactor is assigned and the widget is enabled now warns with
`Set interactor and Enabled before changing Interactive mode.` The message
names the properties the call actually changes, and the warning is emitted with the
widget's name attached, like other VTK warnings.
