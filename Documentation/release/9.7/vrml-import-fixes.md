# vtkVRMLImporter general fixes

* Cone defaults to generating on the Z axis (changed from Y) to match standard. i.e. pointing up
* `appearance` fields can now use the `USE` and `DEF` operand, allowing for appearances to be used by multiple objects
* Added support for the `Extrusion` geometry node type including the `beginCap` `endCap` `crossSection` `spine` and `scale` fields.
* Fixed `Transform` node parsing to respect the order of transform operations
* `Transform` nodes can now use the `USE` and `DEF` operand, allowing for cloning all sub-geometry under one `Transform` into another
