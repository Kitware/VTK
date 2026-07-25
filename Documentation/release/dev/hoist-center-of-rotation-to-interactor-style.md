## CenterOfRotation moved up to vtkInteractorStyle

The `CenterOfRotation` property previously defined on
`vtkInteractorStyleManipulator` now lives on the `vtkInteractorStyle` base
class. Existing code is unaffected: the accessors keep the same signatures
and are simply inherited. All interactor styles now expose the property, so
classes outside `VTK::InteractionStyle` — for example camera-manipulating
widgets — can read and write the center of rotation without depending on a
concrete style subclass.
