## vtkAbstractTransform can update attribute only

vtkAbstractTransform has a `TransformNormals` and `TransformVectors` virtual API,
to transform a single array per call.
Transforming data array was available only through the high level `TransformPointsNormalsVectors`
method that forces geometry transformation alongside to the attributes.
You can now transform only data arrays if needed.

Those methods can be overridden, as done in `vtkLinearTransform`.
